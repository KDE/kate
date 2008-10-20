/* Description : Kate CTags plugin
 *
 * Copyright (C) 2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kate_ctags.h"

#include <QFileInfo>
#include <KFileDialog>

#include <kgenericfactory.h>
#include <kmenu.h>
#include <kactioncollection.h>
#include <kstringhandler.h>
#include <kmessagebox.h>

#define DEFAULT_CTAGS_CMD "ctags -R --c++-types=+px --excmd=pattern --exclude=Makefile --exclude=."

K_EXPORT_COMPONENT_FACTORY(katectagsplugin, KGenericFactory<KateCTagsPlugin>("kate-ctags-plugin"))

/******************************************************************/
KateCTagsView::KateCTagsView(Kate::MainWindow *mw)
    : Kate::PluginView (mw)
        , KXMLGUIClient()
        , m_toolView (mw->createToolView ("kate_private_plugin_katectagsplugin",
                      Kate::MainWindow::Bottom,
                      SmallIcon("application-x-ms-dos-executable"),
                                i18n("CTags"))
                     )
        , m_proc(0)
{
    setComponentData(KComponentData("kate"));

    m_mWin = mw;

    KAction *back = actionCollection()->addAction("ctags_return_step");
    back->setText(i18n("Jump back one step"));
    back->setShortcut(QKeySequence(Qt::ALT+Qt::Key_1) );
    connect(back, SIGNAL(triggered(bool)), this, SLOT(stepBack()));

    KAction *decl = actionCollection()->addAction("ctags_lookup_current_as_declaration");
    decl->setText(i18n("Go to Declaration"));
    decl->setShortcut(QKeySequence(Qt::ALT+Qt::Key_2) );
    connect(decl, SIGNAL(triggered(bool)), this, SLOT(gotoDeclaration()));

    KAction *defin = actionCollection()->addAction("ctags_lookup_current_as_definition");
    defin->setText(i18n("Go to Definition"));
    defin->setShortcut(QKeySequence(Qt::ALT+Qt::Key_3) );
    connect(defin, SIGNAL(triggered(bool)), this, SLOT(gotoDefinition()));

    KAction *lookup = actionCollection()->addAction("ctags_lookup_current");
    lookup->setText(i18n("Lookup Current Text"));
    lookup->setShortcut(QKeySequence(Qt::ALT+Qt::Key_4) );
    connect(lookup, SIGNAL(triggered(bool)), this, SLOT(lookupTag()));

    // popup menu
    m_menu = new KActionMenu(i18n("CTags"), this);
    actionCollection()->addAction("popup_ctags", m_menu);

    m_gotoDec=m_menu->menu()->addAction(i18n("Go to Declaration: %1",QString()), this, SLOT(gotoDeclaration()));
    m_gotoDef=m_menu->menu()->addAction(i18n("Go to Definition: %1",QString()), this, SLOT(gotoDefinition()));
    m_lookup=m_menu->menu()->addAction(i18n("Lookup: %1",QString()), this, SLOT(lookupTag()));

    connect(m_menu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShow()));

    QWidget *ctagsWidget = new QWidget(m_toolView);
    m_ctagsUi.setupUi(ctagsWidget);
    m_ctagsUi.cmdEdit->setText(DEFAULT_CTAGS_CMD);
    m_ctagsUi.loadFile->setText("");
    m_ctagsUi.loadFile->setToolTip(i18n("Load a CTags database file."));
    m_ctagsUi.loadFile->setIcon(KIcon("document-open"));
    m_ctagsUi.newDB->setText("");
    m_ctagsUi.newDB->setToolTip(i18n("Create a CTags database file."));
    m_ctagsUi.newDB->setIcon(KIcon("document-new"));
    m_ctagsUi.updateDB->setText("");
    m_ctagsUi.updateDB->setToolTip(i18n("Re-generate the CTags database file."));
    m_ctagsUi.updateDB->setIcon(KIcon("view-refresh"));

    connect(m_ctagsUi.newDB, SIGNAL(clicked()), this, SLOT(newTagsDB()));
    connect(m_ctagsUi.updateDB, SIGNAL(clicked()), this, SLOT(updateDB()));

    connect(m_ctagsUi.loadFile, SIGNAL(clicked()), this, SLOT(selectTagFile()));
    connect(m_ctagsUi.tagsFile, SIGNAL(returnPressed(QString)), this, SLOT(setTagsFile(QString)));
    connect(m_ctagsUi.tagsFile, SIGNAL(textChanged(QString)), this, SLOT(startTagFileTmr()));

    m_dbTimer.setSingleShot(true);
    connect(&m_dbTimer, SIGNAL(timeout()), this, SLOT(setTagsFile()));

    connect(m_ctagsUi.input_edit, SIGNAL(textChanged(QString)), this, SLOT(startEditTmr()));
    m_editTimer.setSingleShot(true);
    connect(&m_editTimer, SIGNAL(timeout()), this, SLOT(editLookUp()));

    connect(&m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(generateDone()));

    connect(m_ctagsUi.treeWidget, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            SLOT(tagHitClicked(QTreeWidgetItem *)));

    setXMLFile(QString::fromLatin1("plugins/katectags/ui.rc"));
    mainWindow()->guiFactory()->addClient(this);

}



/******************************************************************/
KateCTagsView::~KateCTagsView()
{
    mainWindow()->guiFactory()->removeClient( this );

    delete m_toolView;
}

/******************************************************************/
QWidget *KateCTagsView::toolView() const
{
    return m_toolView;
}

/******************************************************************/
void KateCTagsView::aboutToShow()
{
    QString currWord = currentWord();
    if (currWord.isEmpty()) {
        return;
    }

    if (Tags::hasTag(currWord)) {
        QString squeezed = KStringHandler::csqueeze(currWord, 30);

        m_gotoDec->setText(i18n("Go to Declaration: %1",squeezed));
        m_gotoDef->setText(i18n("Go to Definition: %1",squeezed));
        m_lookup->setText(i18n("Lookup: %1",squeezed));
    }
}


/******************************************************************/
void KateCTagsView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":ctags-plugin");

    m_ctagsUi.cmdEdit->setText(cg.readEntry("TagsGenCMD", DEFAULT_CTAGS_CMD));

    QString tagsFile = cg.readEntry("TagsDatabase", QString());
    QFileInfo file(tagsFile);
    if (file.exists() && file.isFile()) {
      m_ctagsUi.tagsFile->setText(tagsFile);
    }
    else {
      m_ctagsUi.tagsFile->setText("");
    }

    setTagsFile(m_ctagsUi.tagsFile->text());
}

/******************************************************************/
void KateCTagsView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":ctags-plugin");
    cg.writeEntry("TagsDatabase", m_ctagsUi.tagsFile->text());
    cg.writeEntry("TagsGenCMD",m_ctagsUi.cmdEdit->text());
}


/******************************************************************/
void KateCTagsView::stepBack()
{
    if (m_jumpStack.isEmpty()) {
        return;
    }

    TagJump back;
    back = m_jumpStack.pop();

    m_mWin->openUrl(back.url);
    m_mWin->activeView()->setCursorPosition(back.cursor);
    m_mWin->activeView()->setFocus();

}


/******************************************************************/
void KateCTagsView::lookupTag( )
{
    QString currWord = currentWord();
    if (currWord.isEmpty()) {
        return;
    }

    if (!ctagsDBExists()) {
        return;
    }

    clearInput();
    displayHits(Tags::getExactMatches(currWord));

    // activate the hits tab
    m_ctagsUi.tabWidget->setCurrentIndex(0);
    m_mWin->showToolView(m_toolView);
}

/******************************************************************/
void KateCTagsView::editLookUp()
{
    if (!ctagsDBExists()) {
        return;
    }
    displayHits(Tags::getPartialMatches(m_ctagsUi.input_edit->text()));
}

/******************************************************************/
void KateCTagsView::gotoDefinition( )
{
    QString currWord = currentWord();
    if (currWord.isEmpty()) {
        return;
    }

    QStringList types;
    types << "S" << "d" << "f" << "t" << "v";
    gotoTagForTypes(currWord, types);
}

/******************************************************************/
void KateCTagsView::gotoDeclaration( )
{
    QString currWord = currentWord();
    if (currWord.isEmpty()) {
        return;
    }

    QStringList types;
    types << "L" << "c" << "e" << "g" << "m" << "n" << "p" << "s" << "u" << "x";
    gotoTagForTypes(currWord, types);
}

/******************************************************************/
bool KateCTagsView::ctagsDBExists()
{
    if (Tags::getTagsFile().isEmpty()) {
        KGuiItem createNew =
                KGuiItem(i18nc("Button text for creating a new CTags database file.","Create New"),
                         "document-new",
                         i18n("Select a location and create a new CTags database."));
        KGuiItem load =
                KGuiItem(i18nc("Button text for loading a CTags database file","Load"),
                         "document-open",
                         i18n("Select an existing CTags database."));
        int res = KMessageBox::messageBox(0, KMessageBox::QuestionYesNoCancel, i18n("No CTags database is loaded!"), i18n("CTags"),
                                          createNew, load, KStandardGuiItem::cancel());

        if (res == KMessageBox::Yes) {
            newTagsDB();
        }
        else if (res == KMessageBox::No) {
            selectTagFile();
        }

        if (Tags::getTagsFile().isEmpty()) {
            // no database has been created
            return false;
        }
    }
    return true;
}

/******************************************************************/
void KateCTagsView::gotoTagForTypes(const QString &word, const QStringList &types)
{
    if (!ctagsDBExists()) {
        return;
    }

    Tags::TagList list = Tags::getMatches(word, false, types);
    //kDebug(13040) << "found" << list.count() << word << types;

    if ( list.count() < 1) {
        //kDebug(13040) << "No hits found";
        return;
    }

    clearInput();
    displayHits(list);
    if (list.count() == 1) {
        Tags::TagEntry tag = list.first();
        jumpToTag(tag.file, tag.pattern, word);
    }
    else {
        m_ctagsUi.tabWidget->setCurrentIndex(0);
        m_mWin->showToolView(m_toolView);
        KMessageBox::error(0, i18n("Multiple hits found!"));
    }
}

/******************************************************************/
void KateCTagsView::clearInput()
{
    m_ctagsUi.input_edit->blockSignals( true );
    m_ctagsUi.input_edit->clear();
    m_ctagsUi.input_edit->blockSignals( false );
}

/******************************************************************/
void KateCTagsView::displayHits(const Tags::TagList &list)
{
    KUrl url;

    m_ctagsUi.treeWidget->clear();

    Tags::TagList::ConstIterator it = list.begin();
    while(it != list.end()) {
        // search for the file
        if('/'==(*it).file[0]) {
        // we have absolute path
            url.setPath((*it).file);
        }
        else {
        // not absolute
            QFileInfo abs(QFileInfo(Tags::getTagsFile()).path()+'/'+(*it).file);
            url.setPath(abs.absoluteFilePath());
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(m_ctagsUi.treeWidget);
        item->setText(0, (*it).tag);
        item->setText(1, (*it).type);
        item->setText(2, url.toLocalFile());

        item->setData(0, Qt::UserRole, (*it).pattern);

        QString pattern = (*it).pattern;
        pattern.replace( "\\/", "/" );
        pattern = pattern.mid(2, pattern.length() - 4);
        pattern = pattern.trimmed();

        item->setData(0, Qt::ToolTipRole, pattern);
        item->setData(1, Qt::ToolTipRole, pattern);
        item->setData(2, Qt::ToolTipRole, pattern);

        ++it;
    }

}

/******************************************************************/
void KateCTagsView::tagHitClicked(QTreeWidgetItem *item)
{
    // get stuff
    const QString file = item->data(2, Qt::DisplayRole).toString();
    const QString pattern = item->data(0, Qt::UserRole).toString();
    const QString word = item->data(0, Qt::DisplayRole).toString();

    jumpToTag(file, pattern, word);
}

/******************************************************************/
QString KateCTagsView::currentWord( )
{
    KTextEditor::View *kv = mainWindow()->activeView();
    if (!kv) {
        kDebug(13040) << "no KTextEditor::View" << endl;
        return QString();
    }

    if (!kv->cursorPosition().isValid()) {
        kDebug(13040) << "cursor not valid!" << endl;
        return QString();
    }

    int line = kv->cursorPosition().line();
    int col = kv->cursorPosition().column();

    QString linestr = kv->document()->line(line);

    int startPos = qMax(qMin(col, linestr.length()-1), 0);
    int endPos = startPos;
    while (startPos >= 0 && (linestr[startPos].isLetterOrNumber() || linestr[startPos] == '_' || linestr[startPos] == '~')) {
        startPos--;
    }
    while (endPos < (int)linestr.length() && (linestr[endPos].isLetterOrNumber() || linestr[endPos] == '_')) {
        endPos++;
    }
    if  (startPos == endPos) {
        kDebug(13040) << "no word found!" << endl;
        return QString();
    }

    //kDebug(13040) << linestr.mid(startPos+1, endPos-startPos-1);
    return linestr.mid(startPos+1, endPos-startPos-1);
}

/******************************************************************/
void KateCTagsView::jumpToTag(const QString &file, const QString &pattern, const QString &word)
{
    KUrl url;

    if (pattern.isEmpty()) return;

    // generate a regexp from the pattern
    // ctags interestingly escapes "/", but apparently nothing else. lets revert that
    QString unescaped = pattern;
    unescaped.replace( "\\/", "/" );

    // most of the time, the ctags pattern has the form /^foo$/
    // but this isn't true for some macro definitions
    // where the form is only /^foo/
    // I have no idea if this is a ctags bug or not, but we have to deal with it

    QString reduced;
    QString escaped;
    QString re_string;

    if (unescaped.endsWith("$/")) {
        reduced = unescaped.mid(2, unescaped.length() - 4);
        escaped = QRegExp::escape(reduced);
        re_string = QString('^' + escaped + '$');
    }
    else {
        reduced = unescaped.mid( 2, unescaped.length() -3 );
        escaped = QRegExp::escape(reduced);
        re_string = QString('^' + escaped);
    }

    QRegExp re(re_string);

    // search for the file
    if('/'==file[0]) {
        // we have absolute path
        url.setPath(file);
    }
    else {
        // not absolute
        QFileInfo abs(QFileInfo(Tags::getTagsFile()).path()+'/'+file);
        url.setPath(abs.absoluteFilePath());
    }

    //kDebug(13040) << url << pattern;

    // save current location
    TagJump from;
    from.url    = m_mWin->activeView()->document()->url();
    from.cursor = m_mWin->activeView()->cursorPosition();
    m_jumpStack.push(from);

    // open/activate the file
    m_mWin->openUrl(url);

    // any view active?
    if (!m_mWin->activeView()) {
        return;
    }

    // look for the line
    QString linestr;
    int line;
    for (line =0; line < m_mWin->activeView()->document()->lines(); line++) {
        linestr = m_mWin->activeView()->document()->line(line);
        if (linestr.indexOf(re) > -1) break;
    }

    // activate the line
    if (line != m_mWin->activeView()->document()->lines()) {
        // line found now look for the collumn
        int column = linestr.indexOf(word) + (word.length()/2);
        m_mWin->activeView()->setCursorPosition(KTextEditor::Cursor(line, column));
    }
    m_mWin->activeView()->setFocus();

}

/******************************************************************/
void KateCTagsView::setTagsFile(const QString &fname)
{
    //kDebug(13040) << "setting tagsFile" << fname;
    QFileInfo file(fname);
    if (file.exists() && file.isFile()) {
        //kDebug(13040) << "File Found -> setting it";
        m_ctagsUi.updateDB->setDisabled(false);
    }
    else {
        m_ctagsUi.updateDB->setDisabled(true);
    }
    Tags::setTagsFile(fname);

}

/******************************************************************/
void KateCTagsView::selectTagFile()
{
    //kDebug(13040) << "";
    KUrl defDir(m_ctagsUi.tagsFile->text());
    QString new_file = KFileDialog::getOpenFileName(defDir);

    if (!new_file.isEmpty()) {
        m_ctagsUi.tagsFile->setText(new_file);
    }
}


/******************************************************************/
void KateCTagsView::startTagFileTmr()
{
    //kDebug(13040) << "";
    if (m_ctagsUi.tagsFile->text().isEmpty()) {
        m_ctagsUi.updateDB->setDisabled(true);
    }
    m_dbTimer.start(500);
}

/******************************************************************/
void KateCTagsView::startEditTmr()
{
    if (m_ctagsUi.input_edit->text().size() > 3) {
        m_editTimer.start(500);
    }
}

/******************************************************************/
void KateCTagsView::setTagsFile()
{
    //kDebug(13040) << "";
    setTagsFile(m_ctagsUi.tagsFile->text());
}

/******************************************************************/
void KateCTagsView::newTagsDB()
{
    KUrl defDir(m_ctagsUi.tagsFile->text());

    QString file = KFileDialog::getExistingDirectory(defDir, 0, i18n("CTags Database Location"));

    if (!file.isEmpty()) {
        file += "/.ctagsdb";
        m_ctagsUi.tagsFile->setText(file);
        generateTagsDB(file);
    }
}

/******************************************************************/
void KateCTagsView::updateDB()
{
    QString file = m_ctagsUi.tagsFile->text();

    if (!file.isEmpty()) {
        generateTagsDB(file);
    }
}

/******************************************************************/
void KateCTagsView::generateTagsDB(const QString &file)
{
    if (m_proc.state() != QProcess::NotRunning) {
        return;
    }

    QString command = m_ctagsUi.cmdEdit->text() + " -f " + file;

    m_proc.setWorkingDirectory(QFileInfo(file).dir().absolutePath ());
    m_proc.setShellCommand(command);
    m_proc.setOutputChannelMode(KProcess::SeparateChannels);
    m_proc.start();

    if(!m_proc.waitForStarted(500)) {
        KMessageBox::error(0, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

/******************************************************************/
void KateCTagsView::generateDone()
{
    QApplication::restoreOverrideCursor();
    m_ctagsUi.updateDB->setDisabled(false);
}

/******************************************************************/
KateCTagsPlugin::KateCTagsPlugin(QObject* parent, const QStringList&):
        Kate::Plugin ((Kate::Application*)parent)
{
}

/******************************************************************/
Kate::PluginView *KateCTagsPlugin::createView(Kate::MainWindow *mainWindow)
{
    return new KateCTagsView(mainWindow);
}





