/* This file is part of the KDE project
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
   Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>


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
*/

#include <QDir>
#include <QTimer>
#include <KLocale>

#include "kateapp.h"
#include "kateappcommands.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "kateviewmanager.h"

KateAppCommands* KateAppCommands::m_instance = 0;

KateAppCommands::KateAppCommands()
    : KTextEditor::Command()
{
    KTextEditor::Editor *editor = KateDocManager::self()->editor();
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(editor);

    if (iface) {
        iface->registerCommand(this);
    }

    re_write.setPattern("w(a)?");
    re_close.setPattern("bd(elete)?|tabc(lose)?");
    re_quit.setPattern("(w)?q?(a)?");
    re_exit.setPattern("x(a)?");
    re_edit.setPattern("e(dit)?|tabe(dit)?|tabnew");
    re_new.setPattern("(v)?new");
    re_split.setPattern("sp(lit)?");
    re_vsplit.setPattern("vs(plit)?");
    re_only.setPattern("on(ly)?");
}

KateAppCommands::~KateAppCommands()
{
    KTextEditor::Editor *editor = KateDocManager::self()->editor();
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(editor);

    if (iface) {
        iface->unregisterCommand(this);
    }

    m_instance = 0;
}

const QStringList& KateAppCommands::cmds()
{
    static QStringList l;

    if (l.empty()) {
        l << "q" << "qa" << "wq" << "wa" << "wqa" << "x" << "xa" << "new"
          << "vnew" << "e" << "edit" << "enew" << "sp" << "split" << "vs"
          << "vsplit" << "only" << "tabe" << "tabedit" << "tabnew" << "bd"
          << "bdelete" << "tabc" << "tabclose";
    }

    return l;
}

bool KateAppCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    QStringList args(cmd.split( QRegExp("\\s+"), QString::SkipEmptyParts)) ;
    QString command( args.takeFirst() );

    KateMainWindow *mainWin = KateApp::self()->activeMainWindow();

    if (re_write.exactMatch(command)) {  //TODO: handle writing to specific file
        if (!re_write.cap(1).isEmpty()) { // [a]ll
            KateDocManager::self()->saveAll();
            msg = i18n("All documents written to disk");
        } else {
            view->document()->documentSave();
            msg = i18n("Document written to disk");
        }
    }
    // Other buffer commands are implemented by the KateFileTree plugin
    else if (re_close.exactMatch(command)) {
        QTimer::singleShot(0, mainWin, SLOT(slotFileClose()));
    }
    else if (re_quit.exactMatch(command)) {
        if (!re_quit.cap(2).isEmpty()) { // a[ll]
            if (!re_quit.cap(1).isEmpty()) { // [w]rite
                KateDocManager::self()->saveAll();
            }
            QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
        } else {
            if (!re_quit.cap(1).isEmpty() && view->document()->isModified()) { // [w]rite
                view->document()->documentSave();
            }

            if (mainWin->viewManager()->count() > 1) {
              QTimer::singleShot(0, mainWin->viewManager(), SLOT(slotCloseCurrentViewSpace()));
            } else {
                if (KateDocManager::self()->documents() > 1)
                  QTimer::singleShot(0, mainWin, SLOT(slotFileClose()));
                else
                  QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
            }
        }
    } else if (re_exit.exactMatch(command)) {
        if (!re_exit.cap(1).isEmpty()) { // a[ll]
            KateDocManager::self()->saveAll();
            QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
        } else {
            if (view->document()->isModified()) {
                view->document()->documentSave();
            }

            if (KateDocManager::self()->documents() > 1)
                QTimer::singleShot(0, mainWin, SLOT(slotFileClose()));
            else
                QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
        }
        QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
    }
    else if (re_edit.exactMatch(command)) {
        QString argument = args.join(QString(' '));
        if (argument.isEmpty() || argument == "!") {
            view->document()->documentReload();
        } else {
            KUrl base = mainWin->activeDocumentUrl();
            KUrl url;
            KUrl arg2path(argument);
            if (base.isValid()) { // first try to use the same path as the current open document has
              url = KUrl(base.resolved(arg2path));  //resolved handles the case where the args is a relative path, and is the same as using KUrl(args) elsewise
            } else { // else use the cwd
              url = KUrl(KUrl(QDir::currentPath() + "/").resolved(arg2path)); // + "/" is needed because of http://lists.qt.nokia.com/public/qt-interest/2011-May/033913.html
            }
            QFileInfo file( url.toLocalFile() );
            KTextEditor::Document *doc = KateDocManager::self()->findDocument( url );
            if (doc) {
                mainWin->viewManager()->activateView( doc );
            } else if (file.exists()) {
                mainWin->viewManager()->openUrl( url, QString(), true );
            } else {
                mainWin->viewManager()->openUrl( KUrl(), QString(), true )->saveAs ( url );
            }
        }
    }
    else if (re_new.exactMatch(command)) {
        if (re_new.cap(1) == "v") { // vertical split
            mainWin->viewManager()->slotSplitViewSpaceVert();
        } else {                    // horizontal split
            mainWin->viewManager()->slotSplitViewSpaceHoriz();
        }
        mainWin->viewManager()->slotDocumentNew();
    }
    else if (command == "enew") {
        mainWin->viewManager()->slotDocumentNew();
    }
    else if (re_split.exactMatch(command)) {
        mainWin->viewManager()->slotSplitViewSpaceHoriz();
    }
    else if (re_vsplit.exactMatch(command)) {
        mainWin->viewManager()->slotSplitViewSpaceVert();
    } else if (re_only.exactMatch(command)) {
      mainWin->viewManager()->slotCloseOtherViews();
    }

    return true;
}

bool KateAppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view);

    if (re_write.exactMatch(cmd)) {
        msg = i18n("<p><b>w/wa &mdash; write document(s) to disk</b></p>"
                   "<p>Usage: <tt><b>w[a]</b></tt></p>"
                   "<p>Writes the current document(s) to disk. "
                   "It can be called in two ways:<br />"
                   " <tt>w</tt> &mdash; writes the current document to disk<br />"
                   " <tt>wa</tt> &mdash; writes all documents to disk.</p>"
                   "<p>If no file name is associated with the document, "
                   "a file dialog will be shown.</p>");
        return true;
    }
    else if (re_quit.exactMatch(cmd)) {
        msg = i18n("<p><b>q/qa/wq/wqa &mdash; [write and] quit</b></p>"
                   "<p>Usage: <tt><b>[w]q[a]</b></tt></p>"
                   "<p>Quits the application. If <tt>w</tt> is prepended, it also writes"
                   " the document(s) to disk. This command "
                   "can be called in several ways:<br />"
                   " <tt>q</tt> &mdash; closes the current view.<br />"
                   " <tt>qa</tt> &mdash; closes all views, effectively quitting the application.<br />"
                   " <tt>wq</tt> &mdash; writes the current document to disk and closes its view.<br />"
                   " <tt>wqa</tt> &mdash; writes all documents to disk and quits.</p>"
                   "<p>In all cases, if the view being closed is the last view, the application quits. "
                   "If no file name is associated with the document and it should be written to disk, "
                   "a file dialog will be shown.</p>");
        return true;
    }
    else if (re_exit.exactMatch(cmd)) {
        msg = i18n("<p><b>x/xa &mdash; write and quit</b></p>"
                   "<p>Usage: <tt><b>x[a]</b></tt></p>"
                   "<p>Saves document(s) and quits (e<b>x</b>its). This command "
                   "can be called in two ways:<br />"
                   " <tt>x</tt> &mdash; closes the current view.<br />"
                   " <tt>xa</tt> &mdash; closes all views, effectively quitting the application.</p>"
                   "<p>In all cases, if the view being closed is the last view, the application quits. "
                   "If no file name is associated with the document and it should be written to disk, "
                   "a file dialog will be shown.</p>"
                   "<p>Unlike the 'w' commands, this command only writes the document if it is modified."
                   "</p>");
        return true;
    }
    else if (re_split.exactMatch(cmd)) {
        msg = i18n("<p><b>sp,split&mdash; Split horizontally the current view into two</b></p>"
                   "<p>Usage: <tt><b>sp[lit]</b></tt></p>"
                   "<p>The result is two views on the same document.</p>");
        return true;
    }
    else if (re_vsplit.exactMatch(cmd)) {
        msg = i18n("<p><b>vs,vsplit&mdash; Split vertically the current view into two</b></p>"
                   "<p>Usage: <tt><b>vs[plit]</b></tt></p>"
                   "<p>The result is two views on the same document.</p>");
        return true;
    }
    else if (re_new.exactMatch(cmd)) {
        msg = i18n("<p><b>[v]new &mdash; split view and create new document</b></p>"
                   "<p>Usage: <tt><b>[v]new</b></tt></p>"
                   "<p>Splits the current view and opens a new document in the new view."
                   " This command can be called in two ways:<br />"
                   " <tt>new</tt> &mdash; splits the view horizontally and opens a new document.<br />"
                   " <tt>vnew</tt> &mdash; splits the view vertically and opens a new document.<br />"
                   "</p>");
        return true;
    }
    else if (re_edit.exactMatch(cmd)) {
        msg = i18n("<p><b>e[dit] &mdash; reload current document</b></p>"
                   "<p>Usage: <tt><b>e[dit]</b></tt></p>"
                   "<p>Starts <b>e</b>diting the current document again. This is useful to re-edit"
                   " the current file, when it has been changed by another program.</p>");
        return true;
    }

    return false;
}
