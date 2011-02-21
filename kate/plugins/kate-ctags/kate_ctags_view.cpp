/* Description : Kate CTags plugin
 *
 * Copyright (C) 2008-2011 by Kare Sars <kare.sars@iki.fi>
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

#include "kate_ctags_view.h"

#include <QFileInfo>
#include <KFileDialog>
#include <QKeyEvent>

#include <kmenu.h>
#include <kactioncollection.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>
#include <kmessagebox.h>


/******************************************************************/
KateCTagsView::KateCTagsView(Kate::MainWindow *mw, const KComponentData& componentData)
    : Kate::PluginView (mw)
        , Kate::XMLGUIClient(componentData)
        , m_toolView (mw->createToolView ("kate_private_plugin_katectagsplugin",
                      Kate::MainWindow::Bottom,
                      SmallIcon("application-x-ms-dos-executable"),
                                i18n("CTags"))
                     )
        , m_proc(0)
{
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

    m_ctagsUi.addButton->setToolTip(i18n("Add a directory to index."));
    m_ctagsUi.addButton->setIcon(KIcon("list-add"));

    m_ctagsUi.delButton->setToolTip(i18n("Remove a directory."));
    m_ctagsUi.delButton->setIcon(KIcon("list-remove"));

    m_ctagsUi.updateButton->setToolTip(i18n("(Re-)generate the session specific CTags database."));
    m_ctagsUi.updateButton->setIcon(KIcon("view-refresh"));

    m_ctagsUi.updateButton2->setToolTip(i18n("(Re-)generate the session specific CTags database."));
    m_ctagsUi.updateButton2->setIcon(KIcon("view-refresh"));

    m_ctagsUi.resetCMD->setIcon(KIcon("view-refresh"));

    m_ctagsUi.tagsFile->setToolTip(i18n("Select new or existing database file."));

    connect(m_ctagsUi.resetCMD, SIGNAL(clicked()), this, SLOT(resetCMD()));
    connect(m_ctagsUi.addButton, SIGNAL(clicked()), this, SLOT(addTagTarget()));
    connect(m_ctagsUi.delButton, SIGNAL(clicked()), this, SLOT(delTagTarget()));
    connect(m_ctagsUi.updateButton,  SIGNAL(clicked()), this, SLOT(updateSessionDB()));
    connect(m_ctagsUi.updateButton2,  SIGNAL(clicked()), this, SLOT(updateSessionDB()));
    connect(&m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), 
            this, SLOT(updateDone(int, QProcess::ExitStatus)));

    connect(m_ctagsUi.inputEdit, SIGNAL(textChanged(QString)), this, SLOT(startEditTmr()));

    m_editTimer.setSingleShot(true);
    connect(&m_editTimer, SIGNAL(timeout()), this, SLOT(editLookUp()));

    connect(m_ctagsUi.tagTreeWidget, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            SLOT(tagHitClicked(QTreeWidgetItem *)));

    m_toolView->installEventFilter(this);

    mainWindow()->guiFactory()->addClient(this);

    m_commonDB = KStandardDirs::locateLocal("appdata", "plugins/katectags/common_db", true);
}


/******************************************************************/
KateCTagsView::~KateCTagsView()
{
    mainWindow()->guiFactory()->removeClient( this );

    delete m_toolView;
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

    int numEntries = cg.readEntry("SessionNumTargets", 0);
    QString nr;
    QString target;
    for (int i=0; i<numEntries; i++) {
        nr = QString("%1").arg(i,3);
        target = cg.readEntry("SessionTarget_"+nr, QString());
        if (!listContains(target)) {
            new QListWidgetItem(target, m_ctagsUi.targetList);
        }
    }
    
    QString sessionDB = cg.readEntry("SessionDatabase", QString());
    if (sessionDB.isEmpty()) {
        sessionDB = KStandardDirs::locateLocal("appdata", "plugins/katectags/session_db_", true);
        sessionDB += QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    }
    m_ctagsUi.tagsFile->setText(sessionDB);

}

/******************************************************************/
void KateCTagsView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":ctags-plugin");

    cg.writeEntry("TagsGenCMD", m_ctagsUi.cmdEdit->text());
    cg.writeEntry("SessionNumTargets", m_ctagsUi.targetList->count());
    
    QString nr;
    for (int i=0; i<m_ctagsUi.targetList->count(); i++) {
        nr = QString("%1").arg(i,3);
        cg.writeEntry("SessionTarget_"+nr, m_ctagsUi.targetList->item(i)->text());
    }

    cg.writeEntry("SessionDatabase", m_ctagsUi.tagsFile->text());

    cg.sync();
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

    setNewLookupText(currWord);
    Tags::TagList list = Tags::getExactMatches(m_ctagsUi.tagsFile->text(), currWord);
    if (list.size() == 0) list = Tags::getExactMatches(m_commonDB, currWord);
    displayHits(list);

    // activate the hits tab
    m_ctagsUi.tabWidget->setCurrentIndex(0);
    m_mWin->showToolView(m_toolView);
}

/******************************************************************/
void KateCTagsView::editLookUp()
{
    Tags::TagList list = Tags::getPartialMatches(m_ctagsUi.tagsFile->text(), m_ctagsUi.inputEdit->text());
    if (list.size() == 0) list = Tags::getPartialMatches(m_commonDB, m_ctagsUi.inputEdit->text());
    displayHits(list);
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
void KateCTagsView::gotoTagForTypes(const QString &word, const QStringList &types)
{
    Tags::TagList list = Tags::getMatches(m_ctagsUi.tagsFile->text(), word, false, types);
    if (list.size() == 0) list = Tags::getMatches(m_commonDB, word, false, types);
 
    //kDebug() << "found" << list.count() << word << types;
    setNewLookupText(word);
    
    if ( list.count() < 1) {
        m_ctagsUi.tagTreeWidget->clear();
        new QTreeWidgetItem(m_ctagsUi.tagTreeWidget, QStringList(i18n("No hits found")));
        m_ctagsUi.tabWidget->setCurrentIndex(0);
        m_mWin->showToolView(m_toolView);
        return;
    }

    displayHits(list);

    if (list.count() == 1) {
        Tags::TagEntry tag = list.first();
        jumpToTag(tag.file, tag.pattern, word);
    }
    else {
        Tags::TagEntry tag = list.first();
        jumpToTag(tag.file, tag.pattern, word);
        m_ctagsUi.tabWidget->setCurrentIndex(0);
        m_mWin->showToolView(m_toolView);
    }
}

/******************************************************************/
void KateCTagsView::setNewLookupText(const QString &newString)
{
    m_ctagsUi.inputEdit->blockSignals( true );
    m_ctagsUi.inputEdit->setText(newString);
    m_ctagsUi.inputEdit->blockSignals( false );
}

/******************************************************************/
void KateCTagsView::displayHits(const Tags::TagList &list)
{
    KUrl url;

    m_ctagsUi.tagTreeWidget->clear();
    if (list.isEmpty()) {
        new QTreeWidgetItem(m_ctagsUi.tagTreeWidget, QStringList(i18n("No hits found")));
        return;
    }
    m_ctagsUi.tagTreeWidget->setSortingEnabled(false);

    Tags::TagList::ConstIterator it = list.begin();
    while(it != list.end()) {
        // search for the file
        QFileInfo file((*it).file);
        if(file.isAbsolute()) {
        // we have absolute path
            url.setPath((*it).file);
        }
        else {
            // not absolute
            QString name = (*it).file;
            name = name.remove(".\\");
            name = name.replace("\\", "/");
            QFileInfo abs(QFileInfo(Tags::getTagsFile()).path()+ '/' + name);
            url.setPath(abs.absoluteFilePath());
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(m_ctagsUi.tagTreeWidget);
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
    m_ctagsUi.tagTreeWidget->setSortingEnabled(true);
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
        kDebug() << "no KTextEditor::View" << endl;
        return QString();
    }

    if (kv->selection()) {
        return kv->selectionText();
    }

    if (!kv->cursorPosition().isValid()) {
        kDebug() << "cursor not valid!" << endl;
        return QString();
    }

    int line = kv->cursorPosition().line();
    int col = kv->cursorPosition().column();
    bool includeColon = m_ctagsUi.cmdEdit->text().contains("--extra=+q");

    QString linestr = kv->document()->line(line);

    int startPos = qMax(qMin(col, linestr.length()-1), 0);
    int endPos = startPos;
    while (startPos >= 0 && (linestr[startPos].isLetterOrNumber() ||
        (linestr[startPos] == ':' && includeColon) ||
        linestr[startPos] == '_' ||
        linestr[startPos] == '~'))
    {
        startPos--;
    }
    while (endPos < (int)linestr.length() && (linestr[endPos].isLetterOrNumber() ||
        (linestr[startPos] == ':' && includeColon) ||
        linestr[endPos] == '_')) {
        endPos++;
    }
    if  (startPos == endPos) {
        kDebug() << "no word found!" << endl;
        return QString();
    }

    //kDebug() << linestr.mid(startPos+1, endPos-startPos-1);
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
    QFileInfo find(file);
    if(find.isAbsolute()) {
        // we have absolute path
        url.setPath(file);
    }
    else {
        // not absolute
        QString name = file;
        name = name.remove(".\\");
        name = name.replace("\\", "/");
        QFileInfo abs(QFileInfo(Tags::getTagsFile()).path()+ '/' + name);
        url.setPath(abs.absoluteFilePath());
    }

    //kDebug() << url << pattern;

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
void KateCTagsView::startEditTmr()
{
    if (m_ctagsUi.inputEdit->text().size() > 3) {
        m_editTimer.start(500);
    }
}


/******************************************************************/
void KateCTagsView::updateSessionDB()
{
    if (m_proc.state() != QProcess::NotRunning) {
        return;
    }

    QString targets;
    for (int i=0; i<m_ctagsUi.targetList->count(); i++) {
        targets += m_ctagsUi.targetList->item(i)->text() + " ";
    }

    if (m_ctagsUi.tagsFile->text().isEmpty()) {
        // FIXME we need a way to get the session name
        QString sessionDB = KStandardDirs::locateLocal("appdata", "plugins/katectags/session_db_", true);
        sessionDB += QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        m_ctagsUi.tagsFile->setText(sessionDB);
    }

    if (targets.isEmpty()) {
        QFile::remove(m_ctagsUi.tagsFile->text());
        return;
    }

    QString command = QString("%1 -f %2 %3").arg(m_ctagsUi.cmdEdit->text()).arg(m_ctagsUi.tagsFile->text()).arg(targets) ;

    m_proc.setShellCommand(command);
    m_proc.setOutputChannelMode(KProcess::SeparateChannels);
    m_proc.start();

    if(!m_proc.waitForStarted(500)) {
        KMessageBox::error(0, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_ctagsUi.updateButton->setDisabled(true);
    m_ctagsUi.updateButton2->setDisabled(true);
}


/******************************************************************/
void KateCTagsView::updateDone(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        KMessageBox::error(m_toolView, i18n("The CTags executable crashed."));
        return;
    }
    
    if (exitCode != 0) {
        KMessageBox::error(m_toolView, i18n("The CTags program exited with code %1", exitCode));
        return;
    }

    m_ctagsUi.updateButton->setDisabled(false);
    m_ctagsUi.updateButton2->setDisabled(false);
    QApplication::restoreOverrideCursor();
}

/******************************************************************/
void KateCTagsView::addTagTarget()
{
    KUrl defDir = m_mWin->activeView()->document()->url().directory();
    
    KFileDialog dialog(defDir, QString(), 0, 0);
    dialog.setMode(KFile::Directory | KFile::Files | KFile::ExistingOnly | KFile::LocalOnly);
    
    // i18n("CTags Database Location"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QStringList urls = dialog.selectedFiles();

    for (int i=0; i<urls.size(); i++) {
        if (!listContains(urls[i])) {
            new QListWidgetItem(urls[i], m_ctagsUi.targetList);
        }
    }
}

/******************************************************************/
void KateCTagsView::delTagTarget()
{
    delete m_ctagsUi.targetList->currentItem ();
}

/******************************************************************/
bool KateCTagsView::listContains(const QString &target)
{
    for (int i=0; i<m_ctagsUi.targetList->count(); i++) {
        if (m_ctagsUi.targetList->item(i)->text() == target) {
            return true;
        }
    }
    return false;
}

/******************************************************************/
bool KateCTagsView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            mainWindow()->hideToolView(m_toolView);
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/******************************************************************/
void KateCTagsView::resetCMD()
{
    m_ctagsUi.cmdEdit->setText(DEFAULT_CTAGS_CMD);
}

