/*
   This file is part of the Kate text editor of the KDE project.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
*/
// TODO
// Icons
// Direct shortcut setting
#include "externaltools.h"
#include "kateexternaltool.h"
#include "externaltoolsplugin.h"
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
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QStandardPaths>

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

// BEGIN KateExternalToolAction
KateExternalToolAction::KateExternalToolAction(QObject* parent, KateExternalTool* t)
    : QAction(QIcon::fromTheme(t->icon), t->name, parent)
    , tool(t)
{
    setText(t->name);
    if (!t->icon.isEmpty())
        setIcon(QIcon::fromTheme(t->icon));

    connect(this, SIGNAL(triggered(bool)), SLOT(slotRun()));
}

KateExternalToolAction::~KateExternalToolAction()
{
    delete (tool);
}

void KateExternalToolAction::slotRun()
{
    // expand the macros in command if any,
    // and construct a command with an absolute path
    auto mw = qobject_cast<KTextEditor::MainWindow*>(parent()->parent());

    // save documents if requested
    if (tool->saveMode == KateExternalTool::SaveMode::CurrentDocument) {
        mw->activeView()->document()->save();
    } else if (tool->saveMode == KateExternalTool::SaveMode::AllDocuments) {
        foreach (KXMLGUIClient* client, mw->guiFactory()->clients()) {
            if (QAction* a = client->actionCollection()->action(QStringLiteral("file_save_all"))) {
                a->trigger();
                break;
            }
        }
    }

    // copy tool
    auto copy = new KateExternalTool(*tool);

    MacroExpander macroExpander(mw->activeView());
    if (!macroExpander.expandMacrosShellQuote(copy->arguments)) {
        KMessageBox::sorry(mw->activeView(), i18n("Failed to expand the arguments '%1'.", copy->arguments), i18n("Kate External Tools"));
        return;
    }

    if (!macroExpander.expandMacrosShellQuote(copy->workingDir)) {
        KMessageBox::sorry(mw->activeView(), i18n("Failed to expand the working directory '%1'.", copy->workingDir), i18n("Kate External Tools"));
        return;
    }

    // FIXME: The tool runner must live as long as the child process is running.
    //        --> it must be allocated on the heap, and deleted with a ->deleteLater() call.
    KateToolRunner runner(copy);
    runner.run();
    runner.waitForFinished();
}
// END KateExternalToolAction

// BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction(const QString& text, KActionCollection* collection,
                                                         QObject* parent, KTextEditor::MainWindow* mw)
    : KActionMenu(text, parent)
    , mainwindow(mw)
{

    m_actionCollection = collection;

    // connect to view changed...
    connect(mw, &KTextEditor::MainWindow::viewChanged, this, &KateExternalToolsMenuAction::slotViewChanged);

    reload();
}

KateExternalToolsMenuAction::~KateExternalToolsMenuAction()
{
    // kDebug() << "deleted KateExternalToolsMenuAction";
}

void KateExternalToolsMenuAction::reload()
{
    bool needs_readd = (m_actionCollection->takeAction(this) != nullptr);
    m_actionCollection->clear();
    if (needs_readd)
        m_actionCollection->addAction(QStringLiteral("tools_external"), this);
    menu()->clear();

    // load all the tools, and create a action for each of them
    KSharedConfig::Ptr pConfig = KSharedConfig::openConfig(QStringLiteral("externaltools"), KConfig::NoGlobals,
                                                           QStandardPaths::ApplicationsLocation);
    KConfigGroup config(pConfig, "Global");
    const QStringList tools = config.readEntry("tools", QStringList());

    for (int i = 0; i < tools.size(); ++i) {
        const QString & toolSection = tools[i];
        if (toolSection == QStringLiteral("---")) {
            menu()->addSeparator();
            // a separator
            continue;
        }

        config = KConfigGroup(pConfig, toolSection);
        KateExternalTool* t = new KateExternalTool();
        t->load(config);

        if (t->hasexec) {
            QAction* a = new KateExternalToolAction(this, t);
            m_actionCollection->addAction(t->actionName, a);
            addAction(a);
        } else
            delete t;
    }

    config = KConfigGroup(pConfig, "Shortcuts");
    m_actionCollection->readSettings(&config);
    slotViewChanged(mainwindow->activeView());
}

void KateExternalToolsMenuAction::slotViewChanged(KTextEditor::View* view)
{
    // no active view, oh oh
    if (!view) {
        return;
    }

    // try to enable/disable to match current mime type
    KTextEditor::Document* doc = view->document();
    if (doc) {
        const QString mimeType = doc->mimeType();
        foreach (QAction* kaction, m_actionCollection->actions()) {
            KateExternalToolAction* action = dynamic_cast<KateExternalToolAction*>(kaction);
            if (action) {
                const QStringList l = action->tool->mimetypes;
                const bool b = (!l.count() || l.contains(mimeType));
                action->setEnabled(b);
            }
        }
    }
}
// END KateExternalToolsMenuAction

// BEGIN ToolItem
/**
 * This is a QListBoxItem, that has a KateExternalTool. The text is the Name
 * of the tool.
 */
class ToolItem : public QListWidgetItem
{
public:
    ToolItem(QListWidget* lb, const QPixmap& icon, KateExternalTool* tool)
        : QListWidgetItem(icon, tool->name, lb)
        , tool(tool)
    {
    }

    ~ToolItem() {}

    KateExternalTool* tool;
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
        QMessageBox::information(this, i18n("External Tool"), i18n("You must specify at least a name and an executable"));
        return;
    }
    accept();
}

void KateExternalToolServiceEditor::showMTDlg()
{
    QString text = i18n("Select the MimeTypes for which to enable this tool.");
    QStringList list = ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
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

    btnMoveUp->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    btnMoveDown->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

    connect(lbTools, &QListWidget::itemSelectionChanged, this, &KateExternalToolsConfigWidget::slotSelectionChanged);
    connect(lbTools, &QListWidget::itemDoubleClicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnNew, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotNew);
    connect(btnRemove, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotRemove);
    connect(btnEdit, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(btnSeparator, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotInsertSeparator);
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
    // m_tools.clear();
    lbTools->clear();

    // load the files from a KConfig
    const QStringList tools = m_config->group("Global").readEntry("tools", QStringList());

    for (int i = 0; i < tools.size(); ++i) {
        const QString & toolSection = tools[i];
        if (toolSection == QStringLiteral("---")) {
            new QListWidgetItem(QStringLiteral("---"), lbTools);
        } else {
            KConfigGroup cg(m_config, toolSection);
            KateExternalTool* t = new KateExternalTool();
            t->load(cg);

            if (t->hasexec) // we only show tools that are also in the menu.
                new ToolItem(lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t);
            else
                delete t;
        }
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
    for (int i = 0; i < lbTools->count(); i++) {
        if (lbTools->item(i)->text() == QStringLiteral("---")) {
            tools << QStringLiteral("---");
            continue;
        }
        const QString toolSection = QStringLiteral("Tool ") + QString::number(i);
        tools << toolSection;

        KConfigGroup cg(m_config, toolSection);
        KateExternalTool* t = static_cast<ToolItem*>(lbTools->item(i))->tool;
        t->save(cg);
    }

    m_config->group("Global").writeEntry("tools", tools);

    m_config->sync();
    m_plugin->reload();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
    // update button state
    bool hs = lbTools->currentItem() != nullptr;
    btnEdit->setEnabled(hs && dynamic_cast<ToolItem*>(lbTools->currentItem()));
    btnRemove->setEnabled(hs);
    btnMoveUp->setEnabled((lbTools->currentRow() > 0) && hs);
    btnMoveDown->setEnabled((lbTools->currentRow() < (int)lbTools->count() - 1) && hs);
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
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();

        // This is sticky, it does not change again, so that shortcuts sticks
        // TODO check for dups
        t->actionName = QStringLiteral("externaltool_") + QString(t->name).remove(QRegExp(QStringLiteral("\\W+")));

        new ToolItem(lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t);

        emit changed();
        m_changed = true;
    }
}

void KateExternalToolsConfigWidget::slotRemove()
{
    // add the tool action name to a list of removed items,
    // remove the current listbox item
    if (lbTools->currentRow() > -1) {
        ToolItem* i = dynamic_cast<ToolItem*>(lbTools->currentItem());
        if (i)
            m_removed << i->tool->actionName;

        delete lbTools->takeItem(lbTools->currentRow());
        emit changed();
        m_changed = true;
    }
}

void KateExternalToolsConfigWidget::slotEdit()
{
    if (!dynamic_cast<ToolItem*>(lbTools->currentItem()))
        return;
    // show the item in an editor
    KateExternalTool* t = static_cast<ToolItem*>(lbTools->currentItem())->tool;
    KateExternalToolServiceEditor editor(t, this);
    editor.resize(m_config->group("Editor").readEntry("Size", QSize()));
    if (editor.exec() /*== KDialog::Ok*/) {

        bool elementChanged = ((editor.ui->btnIcon->icon() != t->icon) || (editor.ui->edtName->text() != t->name));

        t->name = editor.ui->edtName->text();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->arguments = editor.ui->edtArgs->text();
        t->input = editor.ui->edtInput->toPlainText();
        t->cmdname = editor.ui->edtCommand->text();
        t->workingDir = editor.ui->edtWorkingDir->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegExp(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        t->includeStderr = editor.ui->chkIncludeStderr->isChecked();

        // if the icon has changed or name changed, I have to renew the listbox item :S
        if (elementChanged) {
            int idx = lbTools->row(lbTools->currentItem());
            delete lbTools->takeItem(idx);
            lbTools->insertItem(idx, new ToolItem(nullptr, t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t));
        }

        emit changed();
        m_changed = true;
    }

    m_config->group("Editor").writeEntry("Size", editor.size());
    m_config->sync();
}

void KateExternalToolsConfigWidget::slotInsertSeparator()
{
    lbTools->insertItem(lbTools->currentRow() + 1, QStringLiteral("---"));
    emit changed();
    m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveUp()
{
    // move the current item in the listbox upwards if possible
    QListWidgetItem* item = lbTools->currentItem();
    if (!item)
        return;

    int idx = lbTools->row(item);

    if (idx < 1)
        return;

    if (dynamic_cast<ToolItem*>(item)) {
        KateExternalTool* tool = static_cast<ToolItem*>(item)->tool;
        delete lbTools->takeItem(idx);
        lbTools->insertItem(idx - 1,
                            new ToolItem(nullptr, tool->icon.isEmpty() ? blankIcon() : SmallIcon(tool->icon), tool));
    } else // a separator!
    {
        delete lbTools->takeItem(idx);
        lbTools->insertItem(idx - 1, new QListWidgetItem(QStringLiteral("---")));
    }

    lbTools->setCurrentRow(idx - 1);
    slotSelectionChanged();
    emit changed();
    m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveDown()
{
    // move the current item in the listbox downwards if possible
    QListWidgetItem* item = lbTools->currentItem();
    if (!item)
        return;

    int idx = lbTools->row(item);

    if (idx > lbTools->count() - 1)
        return;

    if (dynamic_cast<ToolItem*>(item)) {
        KateExternalTool* tool = static_cast<ToolItem*>(item)->tool;
        delete lbTools->takeItem(idx);
        lbTools->insertItem(idx + 1,
                            new ToolItem(nullptr, tool->icon.isEmpty() ? blankIcon() : SmallIcon(tool->icon), tool));
    } else // a separator!
    {
        delete lbTools->takeItem(idx);
        lbTools->insertItem(idx + 1, new QListWidgetItem(QStringLiteral("---")));
    }

    lbTools->setCurrentRow(idx + 1);
    slotSelectionChanged();
    emit changed();
    m_changed = true;
}
// END KateExternalToolsConfigWidget

ExternalToolRunner::ExternalToolRunner(KateExternalTool * tool)
    : m_tool(tool)
{
}

ExternalToolRunner::~ExternalToolRunner()
{
}

void ExternalToolRunner::run()
{
}

// kate: space-indent on; indent-width 4; replace-tabs on;
