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
#include <KRun>
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
#include <QRegExp>
#include <QTextEdit>
#include <QToolButton>

#include <unistd.h>

// BEGIN KateExternalToolsCommand
KateExternalToolsCommand::KateExternalToolsCommand(KateExternalToolsPlugin* plugin)
    : KTextEditor::Command({/* FIXME */})
    , m_plugin(plugin)
{
    reload();
}

// FIXME
// const QStringList& KateExternalToolsCommand::cmds()
// {
//     return m_list;
// }

void KateExternalToolsCommand::reload()
{
    m_list.clear();
    m_map.clear();
    m_name.clear();

    KConfig _config(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    KConfigGroup config(&_config, "Global");
    const QStringList tools = config.readEntry("tools", QStringList());

    for (QStringList::const_iterator it = tools.begin(); it != tools.end(); ++it) {
        if (*it == QStringLiteral("---"))
            continue;

        config = KConfigGroup(&_config, *it);

        KateExternalTool t
            = KateExternalTool(config.readEntry(QStringLiteral("name"), ""), config.readEntry("command", ""),
                               config.readEntry(QStringLiteral("icon"), ""), config.readEntry("executable", ""),
                               config.readEntry(QStringLiteral("mimetypes"), QStringList()),
                               config.readEntry(QStringLiteral("acname"), ""), config.readEntry("cmdname", ""));
        // FIXME test for a command name first!
        if (t.hasexec && (!t.cmdname.isEmpty())) {
            m_list.append(QStringLiteral("exttool-") + t.cmdname);
            m_map.insert(QStringLiteral("exttool-") + t.cmdname, t.acname);
            m_name.insert(QStringLiteral("exttool-") + t.cmdname, t.name);
        }
    }
}

bool KateExternalToolsCommand::exec(KTextEditor::View* view, const QString& cmd, QString& msg,
                                    const KTextEditor::Range& range)
{
    Q_UNUSED(msg)
    Q_UNUSED(range)

    auto wv = dynamic_cast<QWidget*>(view);
    if (!wv) {
        //   qDebug()<<"KateExternalToolsCommand::exec: Could not get view widget";
        return false;
    }

    //  qDebug()<<"cmd="<<cmd.trimmed();
    const QString actionName = m_map[cmd.trimmed()];
    if (actionName.isEmpty())
        return false;
    //  qDebug()<<"actionName is not empty:"<<actionName;
    /*  KateExternalToolsMenuAction *a =
        dynamic_cast<KateExternalToolsMenuAction*>(dmw->action("tools_external"));
      if (!a) return false;*/
    KateExternalToolsPluginView* extview = m_plugin->extView(wv->window());
    if (!extview)
        return false;
    if (!extview->externalTools)
        return false;
    //  qDebug()<<"trying to find action";
    QAction* a1 = extview->externalTools->actionCollection()->action(actionName);
    if (!a1)
        return false;
    //  qDebug()<<"activating action";
    a1->trigger();
    return true;
}

bool KateExternalToolsCommand::help(KTextEditor::View*, const QString&, QString&)
{
    return false;
}
// END KateExternalToolsCommand

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

bool KateExternalToolAction::expandMacro(const QString& str, QStringList& ret)
{
    KTextEditor::MainWindow* mw = qobject_cast<KTextEditor::MainWindow*>(parent()->parent());
    Q_ASSERT(mw);

    KTextEditor::View* view = mw->activeView();
    if (!view)
        return false;

    KTextEditor::Document* doc = view->document();
    QUrl url = doc->url();

    if (str == QStringLiteral("URL"))
        ret += url.url();
    else if (str == QStringLiteral("directory")) // directory of current doc
        ret += url.toString(QUrl::RemoveScheme | QUrl::RemoveFilename);
    else if (str == QStringLiteral("filename"))
        ret += url.fileName();
    else if (str == QStringLiteral("line")) // cursor line of current doc
        ret += QString::number(view->cursorPosition().line());
    else if (str == QStringLiteral("col")) // cursor col of current doc
        ret += QString::number(view->cursorPosition().column());
    else if (str == QStringLiteral("selection")) // selection of current doc if any
        ret += view->selectionText();
    else if (str == QStringLiteral("text")) // text of current doc
        ret += doc->text();
    else if (str == QStringLiteral("URLs")) {
        foreach (KTextEditor::Document* it, KTextEditor::Editor::instance()->application()->documents())
            if (!it->url().isEmpty())
                ret += it->url().url();
    } else
        return false;

    return true;
}

KateExternalToolAction::~KateExternalToolAction()
{
    delete (tool);
}

void KateExternalToolAction::slotRun()
{
    // expand the macros in command if any,
    // and construct a command with an absolute path
    QString cmd = tool->command;

    auto mw = qobject_cast<KTextEditor::MainWindow*>(parent()->parent());
    if (!expandMacrosShellQuote(cmd)) {
        KMessageBox::sorry(mw->window(), i18n("Failed to expand the command '%1'.", cmd), i18n("Kate External Tools"));
        return;
    }
    qDebug() << "externaltools: Running command: " << cmd;

    // save documents if requested
    if (tool->saveMode == KateExternalTool::SaveMode::CurrentDocument)
        mw->activeView()->document()->save();
    else if (tool->saveMode == KateExternalTool::SaveMode::AllDocuments) {
        foreach (KXMLGUIClient* client, mw->guiFactory()->clients()) {
            if (QAction* a = client->actionCollection()->action(QStringLiteral("file_save_all"))) {
                a->trigger();
                break;
            }
        }
    }

    KRun::runCommand(cmd, tool->executable, tool->icon, mw->window());
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
    QStringList tools = config.readEntry("tools", QStringList());

    // if there are tools that are present but not explicitly removed,
    // add them to the end of the list
    pConfig->setReadDefaults(true);
    QStringList dtools = config.readEntry("tools", QStringList());
    int gver = config.readEntry("version", 1);
    pConfig->setReadDefaults(false);

    int ver = config.readEntry("version", 0);
    if (ver <= gver) {
        QStringList removed = config.readEntry("removed", QStringList());
        bool sepadded = false;
        for (QStringList::iterator itg = dtools.begin(); itg != dtools.end(); ++itg) {
            if (!tools.contains(*itg) && !removed.contains(*itg)) {
                if (!sepadded) {
                    tools << QStringLiteral("---");
                    sepadded = true;
                }
                tools << *itg;
            }
        }

        config.writeEntry("tools", tools);
        config.sync();
        config.writeEntry("version", gver);
    }

    for (QStringList::const_iterator it = tools.constBegin(); it != tools.constEnd(); ++it) {
        if (*it == QStringLiteral("---")) {
            menu()->addSeparator();
            // a separator
            continue;
        }

        config = KConfigGroup(pConfig, *it);

        KateExternalTool* t = new KateExternalTool(
            config.readEntry("name", ""), config.readEntry("command", ""), config.readEntry("icon", ""),
            config.readEntry("executable", ""), config.readEntry("mimetypes", QStringList()),
            config.readEntry("acname", ""), config.readEntry("cmdname", ""), static_cast<KateExternalTool::SaveMode>(config.readEntry("save", 0)));

        if (t->hasexec) {
            QAction* a = new KateExternalToolAction(this, t);
            m_actionCollection->addAction(t->acname, a);
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
        ui->edtInput->setText(tool->command);
        ui->edtMimeType->setText(tool->mimetypes.join(QStringLiteral("; ")));
        ui->cmbSave->setCurrentIndex(static_cast<int>(tool->saveMode));
        ui->edtCommand->setText(tool->cmdname);
    }
}

void KateExternalToolServiceEditor::slotOKClicked()
{
    if (ui->edtName->text().isEmpty() || ui->edtInput->document()->isEmpty()) {
        QMessageBox::information(this, i18n("External Tool"), i18n("You must specify at least a name and a command"));
        return;
    }
    accept();
}

void KateExternalToolServiceEditor::showMTDlg()
{
    QString text = i18n("Select the MimeTypes for which to enable this tool.");
    QStringList list = ui->edtMimeType->text().split(QRegExp(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
    KMimeTypeChooserDialog d(i18n("Select Mime Types"), text, list, QStringLiteral("text"), this);
    if (d.exec() == QDialog::Accepted) {
        ui->edtMimeType->setText(d.chooser()->mimeTypes().join(QStringLiteral(";")));
    }
}
// END KateExternalToolServiceEditor

// BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin)
    : KTextEditor::ConfigPage(parent)
    , m_changed(false)
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

    config = new KConfig(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    reset();
    slotSelectionChanged();
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
    delete config;
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
    const QStringList tools = config->group("Global").readEntry("tools", QStringList());

    for (QStringList::const_iterator it = tools.begin(); it != tools.end(); ++it) {
        if (*it == QStringLiteral("---")) {
            new QListWidgetItem(QStringLiteral("---"), lbTools);
        } else {
            KConfigGroup cg(config, *it);

            KateExternalTool* t
                = new KateExternalTool(cg.readEntry("name", ""), cg.readEntry("command", ""), cg.readEntry("icon", ""),
                                       cg.readEntry("executable", ""), cg.readEntry("mimetypes", QStringList()),
                                       cg.readEntry("acname"), cg.readEntry("cmdname"), static_cast<KateExternalTool::SaveMode>(cg.readEntry("save", 0)));

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

    // save a new list
    // save each item
    QStringList tools;
    for (int i = 0; i < lbTools->count(); i++) {
        if (lbTools->item(i)->text() == QStringLiteral("---")) {
            tools << QStringLiteral("---");
            continue;
        }
        KateExternalTool* t = static_cast<ToolItem*>(lbTools->item(i))->tool;
        //     qDebug()<<"adding tool: "<<t->name;
        tools << t->acname;

        KConfigGroup cg(config, t->acname);

        cg.writeEntry("name", t->name);
        cg.writeEntry("command", t->command);
        cg.writeEntry("icon", t->icon);
        cg.writeEntry("executable", t->executable);
        cg.writeEntry("mimetypes", t->mimetypes);
        cg.writeEntry("acname", t->acname);
        cg.writeEntry("cmdname", t->cmdname);
        cg.writeEntry("save", static_cast<int>(t->saveMode));
    }

    config->group("Global").writeEntry("tools", tools);

    // if any tools was removed, try to delete their groups, and
    // add the group names to the list of removed items.
    if (m_removed.count()) {
        for (QStringList::iterator it = m_removed.begin(); it != m_removed.end(); ++it) {
            if (config->hasGroup(*it))
                config->deleteGroup(*it);
        }
        QStringList removed = config->group("Global").readEntry("removed", QStringList());
        removed += m_removed;

        // clean up the list of removed items, so that it does not contain
        // non-existing groups (we can't remove groups from a non-owned global file).
        config->sync();
        QStringList::iterator it1 = removed.begin();
        while (it1 != removed.end()) {
            if (!config->hasGroup(*it1))
                it1 = removed.erase(it1);
            else
                ++it1;
        }
        config->group("Global").writeEntry("removed", removed);
    }

    config->sync();
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
        KateExternalTool* t = new KateExternalTool(
            editor.ui->edtName->text(), editor.ui->edtInput->toPlainText(), editor.ui->btnIcon->icon(), editor.ui->edtExecutable->text(),
            editor.ui->edtMimeType->text().split(QRegExp(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts));

        // This is sticky, it does not change again, so that shortcuts sticks
        // TODO check for dups
        t->acname = QStringLiteral("externaltool_") + QString(t->name).remove(QRegExp(QStringLiteral("\\W+")));

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
            m_removed << i->tool->acname;

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
    editor.resize(config->group("Editor").readEntry("Size", QSize()));
    if (editor.exec() /*== KDialog::Ok*/) {

        bool elementChanged = ((editor.ui->btnIcon->icon() != t->icon) || (editor.ui->edtName->text() != t->name));

        t->name = editor.ui->edtName->text();
        t->cmdname = editor.ui->edtCommand->text();
        t->command = editor.ui->edtInput->toPlainText();
        t->icon = editor.ui->btnIcon->icon();
        t->executable = editor.ui->edtExecutable->text();
        t->mimetypes = editor.ui->edtMimeType->text().split(QRegExp(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
        t->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());

        // if the icon has changed or name changed, I have to renew the listbox item :S
        if (elementChanged) {
            int idx = lbTools->row(lbTools->currentItem());
            delete lbTools->takeItem(idx);
            lbTools->insertItem(idx, new ToolItem(nullptr, t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t));
        }

        emit changed();
        m_changed = true;
    }

    config->group("Editor").writeEntry("Size", editor.size());
    config->sync();
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
