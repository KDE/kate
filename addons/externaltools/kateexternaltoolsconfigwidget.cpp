/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
// TODO
// Icons
// Direct shortcut setting
#include "kateexternaltoolsconfigwidget.h"
#include "externaltoolsplugin.h"
#include "kateexternaltool.h"
#include "katemacroexpander.h"
#include "katetoolrunner.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KMessageBox>
#include <KMimeTypeChooser>
#include <KSharedConfig>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>
#include <QComboBox>
#include <QTreeView>
#include <QMenu>
#include <QStandardPaths>
#include <QStandardItem>

#include <QBitmap>
#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QToolButton>

#include <unistd.h>

// BEGIN ToolItem
/**
 * This is a QStandardItem, that has a KateExternalTool.
 * The text is the Name of the tool.
 */
class ToolItem : public QStandardItem
{
public:
    ToolItem(const QPixmap& icon, KateExternalTool* tool)
        : QStandardItem(icon, tool->name)
    {
        setData(QVariant::fromValue(tool));
    }

    KateExternalTool * tool() {
        return qvariant_cast<KateExternalTool*>(data());
    }
};
// END ToolItem

// BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor(KateExternalTool* tool, QWidget* parent)
    : QDialog(parent)
    , tool(tool)
{
    setWindowTitle(i18n("Edit External Tool"));

    ui = new Ui::ToolDialog();
    ui->setupUi(this);
    ui->btnIcon->setIconSize(KIconLoader::SizeSmall);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KateExternalToolServiceEditor::slotOKClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->btnMimeType, &QToolButton::clicked, this, &KateExternalToolServiceEditor::showMTDlg);

    if (tool) {
        ui->edtName->setText(tool->name);
        if (!tool->icon.isEmpty())
            ui->btnIcon->setIcon(tool->icon);

        ui->edtExecutable->setText(tool->executable);
        ui->edtArgs->setText(tool->arguments);
        ui->edtInput->setText(tool->input);
        ui->edtCommand->setText(tool->cmdname);
        ui->edtWorkingDir->setText(tool->workingDir);
        ui->edtMimeType->setText(tool->mimetypes.join(QStringLiteral("; ")));
        ui->cmbSave->setCurrentIndex(static_cast<int>(tool->saveMode));
        ui->chkIncludeStderr->setChecked(tool->includeStderr);
    }
}

void KateExternalToolServiceEditor::slotOKClicked()
{
    if (ui->edtName->text().isEmpty() || ui->edtExecutable->text().isEmpty()) {
        QMessageBox::information(this, i18n("External Tool"),
                                 i18n("You must specify at least a name and an executable"));
        return;
    }
    accept();
}

void KateExternalToolServiceEditor::showMTDlg()
{
    QString text = i18n("Select the MimeTypes for which to enable this tool.");
    QStringList list
        = ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
    KMimeTypeChooserDialog d(i18n("Select Mime Types"), text, list, QStringLiteral("text"), this);
    if (d.exec() == QDialog::Accepted) {
        ui->edtMimeType->setText(d.chooser()->mimeTypes().join(QStringLiteral(";")));
    }
}
// END KateExternalToolServiceEditor

// BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    setupUi(this);
    lbTools->setModel(&m_toolsModel);

    btnMoveUp->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    btnMoveDown->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

//    connect(lbTools, &QTreeView::itemSelectionChanged, this, &KateExternalToolsConfigWidget::slotSelectionChanged);
//     connect(lbTools, &QTreeView::itemDoubleClicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnNew, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotNew);
    connect(btnRemove, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotRemove);
    connect(btnEdit, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnMoveUp, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotMoveUp);
    connect(btnMoveDown, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotMoveDown);

    m_config = new KConfig(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    reset();
    slotSelectionChanged();
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
    delete m_config;
}

QString KateExternalToolsConfigWidget::name() const
{
    return i18n("External Tools");
}

QString KateExternalToolsConfigWidget::fullName() const
{
    return i18n("External Tools");
}

QIcon KateExternalToolsConfigWidget::icon() const
{
    return QIcon();
}

void KateExternalToolsConfigWidget::reset()
{
    // clear list
    m_toolsModel.clear();

    // 2 steps: 1st step: create categories
    const auto tools = m_plugin->tools();
    for (auto tool : tools) {
        auto clone = new KateExternalTool(*tool);
        auto item = new ToolItem(clone->icon.isEmpty() ? blankIcon() : SmallIcon(clone->icon), clone);
        auto category = addCategory(clone->category.isEmpty() ? i18n("Uncategorized") : clone->category);
        category->appendRow(item);
        qDebug() << "HANEDLED" << clone->name;
    }
    m_changed = false;
}

QPixmap KateExternalToolsConfigWidget::blankIcon()
{
    QPixmap pm(KIconLoader::SizeSmall, KIconLoader::SizeSmall);
    pm.fill();
    pm.setMask(pm.createHeuristicMask());
    return pm;
}

void KateExternalToolsConfigWidget::apply()
{
    if (!m_changed)
        return;
    m_changed = false;

    QStringList tools;
//     for (int i = 0; i < lbTools->count(); i++) {
//         const QString toolSection = QStringLiteral("Tool ") + QString::number(i);
//         tools << toolSection;
//
//         KConfigGroup cg(m_config, toolSection);
//         KateExternalTool* t = static_cast<ToolItem*>(lbTools->item(i))->tool;
//         t->save(cg);
//     }

    m_config->group("Global").writeEntry("tools", tools);

    m_config->sync();
    m_plugin->reload();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
    // update button state
//    bool hs = lbTools->currentItem() != nullptr;
//    btnEdit->setEnabled(hs && dynamic_cast<ToolItem*>(lbTools->currentItem()));
//    btnRemove->setEnabled(hs);
//    btnMoveUp->setEnabled((lbTools->currentRow() > 0) && hs);
//    btnMoveDown->setEnabled((lbTools->currentRow() < (int)lbTools->count() - 1) && hs);
}

QStandardItem * KateExternalToolsConfigWidget::addCategory(const QString & category)
{
    // searach for existing category
    auto items = m_toolsModel.findItems(category);
    if (!items.empty()) {
        return items.front();
    }

    // ...otherwise, create it
    auto item = new QStandardItem(category);
    m_toolsModel.appendRow(item);
    return item;
}

void KateExternalToolsConfigWidget::slotNew()
{
    // display a editor, and if it is OK'd, create a new tool and
    // create a listbox item for it
    KateExternalToolServiceEditor editor(nullptr, this);

    if (editor.exec() == QDialog::Accepted) {
        KateExternalTool* t = new KateExternalTool();
        t->name = editor.ui->edtName->text();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->arguments = editor.ui->edtArgs->text();
        t->input = editor.ui->edtInput->toPlainText();
        t->workingDir = editor.ui->edtWorkingDir->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")),
                                                            QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();

        // This is sticky, it does not change again, so that shortcuts sticks
        // TODO check for dups
        t->actionName = QStringLiteral("externaltool_") + QString(t->name).remove(QRegularExpression(QStringLiteral("\\W+")));

        auto item = new ToolItem(t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t);
        auto category = addCategory(item->tool()->category);
        category->appendRow(item);

        emit changed();
        m_changed = true;
    }
}

void KateExternalToolsConfigWidget::slotRemove()
{
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    auto toolItem = dynamic_cast<ToolItem*>(item);

    // add the tool action name to a list of removed items,
    // remove the current listbox item
    if (toolItem) {
        m_removed << toolItem->tool()->actionName;

        toolItem->parent()->removeRow(toolItem->index().row());
        delete toolItem;;
        emit changed();
        m_changed = true;
    }
}

void KateExternalToolsConfigWidget::slotEdit()
{
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    auto toolItem = dynamic_cast<ToolItem*>(item);
    if (!toolItem)
        return;
    // show the item in an editor
    KateExternalTool* t = toolItem->tool();
    KateExternalToolServiceEditor editor(t, this);
    editor.resize(m_config->group("Editor").readEntry("Size", QSize()));
    if (editor.exec() /*== KDialog::Ok*/) {

        const bool elementChanged = ((editor.ui->btnIcon->icon() != t->icon) || (editor.ui->edtName->text() != t->name));

        t->name = editor.ui->edtName->text();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->arguments = editor.ui->edtArgs->text();
        t->input = editor.ui->edtInput->toPlainText();
        t->cmdname = editor.ui->edtCommand->text();
        t->workingDir = editor.ui->edtWorkingDir->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();

        // if the icon or name name changed, renew the item
        if (elementChanged) {
            toolItem->setText(t->name);
            toolItem->setIcon(t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon));
        }

        emit changed();
        m_changed = true;
    }

    m_config->group("Editor").writeEntry("Size", editor.size());
    m_config->sync();
}

void KateExternalToolsConfigWidget::slotMoveUp()
{
    // move the current item in the listbox upwards if possible
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
//     auto toolItem = dynamic_cast<ToolItem*>(item);
    if (!item)
        return;

    QModelIndex srcParent = item->parent() ? item->parent()->index() : m_toolsModel.invisibleRootItem()->index();
    int srcRow = item->index().row();
    QModelIndex dstParent = (item->index().row() > 0) ? srcParent : QModelIndex();
    int dstRow = item->index().row() > 0 ? (item->index().row() - 1) : 0;

    bool moved = m_toolsModel.moveRow(srcParent, srcRow, dstParent, dstRow);
    qDebug() << "Moving up succesful?" << moved;

//    slotSelectionChanged();
    emit changed();
    m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveDown()
{
    // move the current item in the listbox downwards if possible
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
//     auto toolItem = dynamic_cast<ToolItem*>(item);
    if (!item)
        return;

//     int idx = lbTools->row(item);
//     if (idx > lbTools->count() - 1)
//         return;

    QModelIndex srcParent = item->parent() ? item->parent()->index() : m_toolsModel.invisibleRootItem()->index();
    int srcRow = item->index().row();
    QModelIndex dstParent = srcParent;
    int dstRow = item->index().row() + 1;

//     slotSelectionChanged();
    emit changed();
    m_changed = true;
}
// END KateExternalToolsConfigWidget

// kate: space-indent on; indent-width 4; replace-tabs on;
