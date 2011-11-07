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

#include <QTimer>
#include <KLocale>

#include "kateapp.h"
#include "kateappcommands.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "kateviewmanager.h"

KateAppCommands::KateAppCommands()
    : KTextEditor::Command()
{
    KTextEditor::Editor *editor = KateDocManager::self()->editor();
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(editor);

    if (iface) {
        iface->registerCommand(this);
    }

    re_write.setPattern("w(a)?");
    re_quit.setPattern("(w)?q?(a)?");
    re_exit.setPattern("x(a)?");
    re_edit.setPattern("e(dit)?");
    re_new.setPattern("(v)?new");
    re_split.setPattern("sp(lit)?");
    re_vsplit.setPattern("vs(plit)?");
    re_bufferNext.setPattern("bn(ext)?");
    re_bufferPrev.setPattern("bp(revious)?");
    re_bufferFirst.setPattern("bf(irst)?");
    re_bufferLast.setPattern("bl(ast)?");
    re_editBuffer.setPattern("b(uffer)?");

}

KateAppCommands::~KateAppCommands()
{
    KTextEditor::Editor *editor = KateDocManager::self()->editor();
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(editor);

    if (iface) {
        iface->unregisterCommand(this);
    }
}

const QStringList& KateAppCommands::cmds()
{
    static QStringList l;

    if (l.empty()) {
        l << "q" << "qa" /*<< "w"*/ << "wq" << "wa" << "wqa" << "x" << "xa"
          << "bn" << "bp" << "new" << "vnew" << "e" << "edit" << "enew"
          << "sp" << "split" << "vs" << "vsplit" << "bn" << "bnext" << "bp"
          << "bprevious" <<  "bf" << "bfirst" << "bl" << "blast" << "b" << "buffer";
    }

    return l;
}

bool KateAppCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    QStringList args(cmd.split( QRegExp("\\s+"), QString::SkipEmptyParts)) ;
    QString command( args.takeFirst() );

    KateMainWindow *mainWin = KateApp::self()->activeMainWindow();

    if (re_write.exactMatch(command)) {
        if (!re_write.cap(1).isEmpty()) { // [a]ll
            KateDocManager::self()->saveAll();
            msg = i18n("All documents written to disk");
        } else {
            view->document()->documentSave();
            msg = i18n("Document written to disk");
        }
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

            if (KateDocManager::self()->documents() > 1)
                QTimer::singleShot(0, mainWin, SLOT(slotFileClose()));
            else
                QTimer::singleShot(0, mainWin, SLOT(slotFileQuit()));
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
        view->document()->documentReload();
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
    }
    else if (re_bufferFirst.exactMatch(command)) {
        qDebug() << "bufferFirst";
        mainWin->viewManager()->activateView( KateDocManager::self()->documentList().first());
    }
    else if (re_bufferLast.exactMatch(command)) {
      mainWin->viewManager()->activateView( KateDocManager::self()->documentList().last());
    }
    else if (re_bufferNext.exactMatch(command) || re_bufferPrev.exactMatch(command)) {
      // skipping 1 document by default
      int count = 1;

      if (args.size() == 1) {
        count = args.at(0).toInt();
      }
      int current_document_position = KateDocManager::self()->findDocument(
                                 mainWin->viewManager()->activeView()->document());

      uint wanted_document_position;

      if (re_bufferPrev.exactMatch(command)) {
          count = -count;
          int mult = qAbs( current_document_position + count  ) / KateDocManager::self()->documents();
          wanted_document_position = (mult + (current_document_position + count < 0))
                                   * KateDocManager::self()->documents() + (current_document_position + count);
      } else {
          int mult =  ( current_document_position + count ) / KateDocManager::self()->documents();
          wanted_document_position = (current_document_position + count) - mult
                                   * KateDocManager::self()->documents();
      }

      if ( wanted_document_position > KateDocManager::self()->documents()) {
        msg = i18n("Cannot go to the document");
      } else {
        mainWin->viewManager()->activateView(KateDocManager::self()->document( wanted_document_position ));
      }

    }
    else if (re_editBuffer.exactMatch(command)) {
        if (args.size() == 1 &&
            args.at(0).toInt() > 0 &&
            args.at(0).toUInt() <= KateDocManager::self()->documents()) {
          mainWin->viewManager()->activateView(
                KateDocManager::self()->document(args.at(0).toInt() - 1));
        }
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
    else if (re_bufferNext.exactMatch(cmd)) {
        msg = i18n("<p><b>bn,bnext &mdash; switch to next document</b></p>"
              "<p>Usage: <tt><b>bn[ext] [N]</b></tt></p>"
              "<p>Goes to <b>[N]</b>th next document (\"<b>b</b>uffer\") in document list."
              "<b>[N]</b> defaults to one. </p>"
              "<p>Wraps around the end of the document list.</p>");
        return true;
    }
    else if (re_bufferPrev.exactMatch(cmd)) {
      msg = i18n("<p><b>bp,bprev &mdash; previous buffer</b></p>"
            "<p>Usage: <tt><b>bp[revious] [N]</b></tt></p>"
            "<p>Goes to <b>[N]</b>th previous document (\"<b>b</b>uffer\") in document list. </p>"
            "<p> <b>[N]</b> defaults to one. </p>"
            "<p>Wraps around the start of the document list.</p>");
      return true;
    }
    else if (re_bufferFirst.exactMatch(cmd)) {
      msg = i18n("<p><b>bf,bfirst &mdash; first document</b></p>"
            "<p>Usage: <tt><b>bf[irst]</b></tt></p>"
            "<p>Goes to the <b>f</b>irst document (\"<b>b</b>uffer\") in document list.</p>");
      return true;
    }
    else if (re_bufferLast.exactMatch(cmd)) {
      msg = i18n("<p><b>bl,blast &mdash; last document</b></p>"
            "<p>Usage: <tt><b>bl[ast]</b></tt></p>"
            "<p>Goes to the <b>l</b>ast document (\"<b>b</b>uffer\") in document list.</p>");
      return true;
    }
    else if (re_editBuffer.exactMatch(cmd)) {
      msg = i18n("<p><b>b,buffer &mdash; Edit document N from the document list</b></p>"
            "<p>Usage: <tt><b>b[uffer] [N]</b></tt></p>");
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
