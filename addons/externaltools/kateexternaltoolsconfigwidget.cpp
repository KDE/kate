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

#include <KConfig>
#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KMimeTypeChooser>
#include <KSharedConfig>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>
#include <QBitmap>
#include <QComboBox>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardItem>
#include <QStandardPaths>
#include <QTreeView>

#include <unistd.h>

namespace {
    constexpr int ToolRole = Qt::UserRole + 1;
}

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
        setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
        setData(QVariant::fromValue(reinterpret_cast<quintptr>(tool)), ToolRole );
    }

    KateExternalTool * tool() {
        return reinterpret_cast<KateExternalTool*>(data(ToolRole).value<quintptr>());
    }
};
// END ToolItem

// BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor(KateExternalTool* tool, QWidget* parent)
    : QDialog(parent)
    , m_tool(tool)
{
    setWindowTitle(i18n("Edit External Tool"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-run")));

    ui = new Ui::ToolDialog();
    ui->setupUi(this);
    ui->btnIcon->setIconSize(KIconLoader::SizeSmall);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KateExternalToolServiceEditor::slotOKClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->btnMimeType, &QToolButton::clicked, this, &KateExternalToolServiceEditor::showMTDlg);

    Q_ASSERT(m_tool != nullptr);
    ui->edtName->setText(m_tool->name);
    if (!m_tool->icon.isEmpty())
        ui->btnIcon->setIcon(m_tool->icon);

    ui->edtExecutable->setText(m_tool->executable);
    ui->edtArgs->setText(m_tool->arguments);
    ui->edtInput->setText(m_tool->input);
    ui->edtWorkingDir->setText(m_tool->workingDir);
    ui->edtMimeType->setText(m_tool->mimetypes.join(QStringLiteral("; ")));
    ui->cmbSave->setCurrentIndex(static_cast<int>(m_tool->saveMode));
    ui->chkReload->setChecked(m_tool->reload);
    ui->cmbOutput->setCurrentIndex(static_cast<int>(m_tool->outputMode));
    ui->chkIncludeStderr->setChecked(m_tool->includeStderr);
    ui->edtCommand->setText(m_tool->cmdname);
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

static std::vector<QStandardItem*> childItems(const QStandardItem * item)
{
    // collect all KateExternalTool items
    std::vector<QStandardItem*> children;
    for (int i = 0; i < item->rowCount(); ++i) {
        children.push_back(item->child(i));
    }
    return children;
}

static std::vector<KateExternalTool*> collectTools(const QStandardItemModel & model)
{
    std::vector<KateExternalTool*> tools;
    for (auto categoryItem : childItems(model.invisibleRootItem())) {
        for (auto child : childItems(categoryItem)) {
            auto toolItem = static_cast<ToolItem*>(child);
            tools.push_back(toolItem->tool());
        }
    }
    return tools;
}

// BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    setupUi(this);
    lbTools->setModel(&m_toolsModel);
    lbTools->setSelectionMode(QAbstractItemView::SingleSelection);
    lbTools->setDragEnabled(true);
    lbTools->setAcceptDrops(true);
    lbTools->setDefaultDropAction(Qt::MoveAction);
    lbTools->setDropIndicatorShown(true);
    lbTools->setDragDropOverwriteMode(false);
    lbTools->setDragDropMode(QAbstractItemView::InternalMove);

    btnMoveUp->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    btnMoveDown->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

    connect(lbTools->selectionModel(), &QItemSelectionModel::currentChanged, [this](){
        slotSelectionChanged();
    });
    connect(lbTools, &QTreeView::doubleClicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnNew, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotNew);
    connect(btnRemove, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotRemove);
    connect(btnEdit, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnMoveUp, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotMoveUp);
    connect(btnMoveDown, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotMoveDown);

    m_config = new KConfig(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);

    // reset triggers a reload of the existing tools
    reset();
    slotSelectionChanged();
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
    clearTools();

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
    return QIcon::fromTheme(QStringLiteral("system-run"));
}

void KateExternalToolsConfigWidget::reset()
{
    clearTools();
    m_toolsModel.invisibleRootItem()->setFlags(Qt::NoItemFlags);

    // the "Uncategorized" category always exists
    m_noCategory = addCategory(i18n("Uncategorized"));
    m_noCategory->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);

    // create other tools and categories
    const auto tools = m_plugin->tools();
    for (auto tool : tools) {
        auto clone = new KateExternalTool(*tool);
        auto item = new ToolItem(clone->icon.isEmpty() ? blankIcon() : SmallIcon(clone->icon), clone);
        auto category = clone->category.isEmpty() ? m_noCategory : addCategory(clone->category);
        category->appendRow(item);
    }
    lbTools->expandAll();
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

    // collect all KateExternalTool items
    std::vector<KateExternalTool*> tools;
    for (auto categoryItem : childItems(m_toolsModel.invisibleRootItem())) {
        const QString category = (categoryItem == m_noCategory) ? QString() : categoryItem->text();
        for (auto child : childItems(categoryItem)) {
            auto toolItem = static_cast<ToolItem*>(child);
            auto tool = toolItem->tool();
            // at this point, we have to overwrite the category, since it may have changed (and we never tracked this)
            tool->category = category;
            tools.push_back(tool);
        }
    }

    // write tool configuration to disk
    m_config->group("Global").writeEntry("tools", static_cast<int>(tools.size()));
    for (size_t i = 0; i < tools.size(); i++) {
        const QString section = QStringLiteral("Tool ") + QString::number(i);
        KConfigGroup cg(m_config, section);
        tools[i]->save(cg);
    }

    m_config->sync();
    m_plugin->reload();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
    // update button state
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    const bool isToolItem = dynamic_cast<ToolItem*>(item) != nullptr;

    btnEdit->setEnabled(isToolItem);
    btnRemove->setEnabled(isToolItem);
//     btnMoveUp->setEnabled((lbTools->currentRow() > 0) && hs);
//     btnMoveDown->setEnabled((lbTools->currentRow() < (int)lbTools->count() - 1) && hs);
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

    // for now, categories are not movable, otherwise, the use can move a
    // category into another category, which is not supported right now
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsEditable);

    m_toolsModel.appendRow(item);
    return item;
}

QStandardItem * KateExternalToolsConfigWidget::currentCategory() const
{
    auto index = lbTools->currentIndex();
    if (!index.isValid()) {
        return m_noCategory;
    }

    auto item = m_toolsModel.itemFromIndex(index);
    auto toolItem = dynamic_cast<ToolItem*>(item);
    if (toolItem) {
        // the parent of a ToolItem is always a category
        return toolItem->parent();
    }

    // item is no ToolItem, so we must have a category at hand
    return item;
}

void KateExternalToolsConfigWidget::clearTools()
{
    // collect all KateExternalTool items and delete them, since they are copies
    std::vector<KateExternalTool*> tools = collectTools(m_toolsModel);
    qDeleteAll(tools);
    tools.clear();
    m_toolsModel.clear();
}

void KateExternalToolsConfigWidget::slotNew()
{
    // display a editor, and if it is OK'd, create a new tool and
    // create a listbox item for it
    auto t = new KateExternalTool();
    KateExternalToolServiceEditor editor(t, this);
    if (editor.exec() == QDialog::Accepted) {
        t->name = editor.ui->edtName->text();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->arguments = editor.ui->edtArgs->text();
        t->input = editor.ui->edtInput->toPlainText();
        t->workingDir = editor.ui->edtWorkingDir->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")),
                                                            QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->reload = editor.ui->chkReload->isChecked();
        t->outputMode = static_cast<KateExternalTool::OutputMode>(editor.ui->cmbOutput->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();
        t->cmdname = editor.ui->edtCommand->text();

        // This is sticky, it does not change again, so that shortcuts sticks
        // TODO check for dups
        t->actionName = QStringLiteral("externaltool_") + QString(t->name).remove(QRegularExpression(QStringLiteral("\\W+")));

        auto item = new ToolItem(t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t);
        auto category = currentCategory();
        category->appendRow(item);
        lbTools->setCurrentIndex(item->index());

        emit changed();
        m_changed = true;
    } else {
        delete t;
    }
}

void KateExternalToolsConfigWidget::slotRemove()
{
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    auto toolItem = dynamic_cast<ToolItem*>(item);

    // add the tool action name to a list of removed items,
    // remove the current listbox item
    if (toolItem) {
        auto tool = toolItem->tool();
        toolItem->parent()->removeRow(toolItem->index().row());
        delete tool;
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
    if (editor.exec()  == QDialog::Accepted) {

        const bool elementChanged = ((editor.ui->btnIcon->icon() != t->icon) || (editor.ui->edtName->text() != t->name));

        t->name = editor.ui->edtName->text();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->arguments = editor.ui->edtArgs->text();
        t->input = editor.ui->edtInput->toPlainText();
        t->workingDir = editor.ui->edtWorkingDir->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->reload = editor.ui->chkReload->isChecked();
        t->outputMode = static_cast<KateExternalTool::OutputMode>(editor.ui->cmbOutput->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();
        t->cmdname = editor.ui->edtCommand->text();

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
