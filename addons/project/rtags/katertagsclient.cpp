/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "katertagsclient.h"

#include <QProcess>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

#include <KTextEditor/Editor>
#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

KateRtagsClient::KateRtagsClient()
    : QObject()
{
}

KateRtagsClient::~KateRtagsClient()
{
}

bool KateRtagsClient::rtagsAvailable() const
{
    return (!QStandardPaths::findExecutable(QStringLiteral("rc")).isEmpty())
        && (!QStandardPaths::findExecutable(QStringLiteral("rdm")).isEmpty());
}

bool KateRtagsClient::rtagsDaemonRunning() const
{
    // Try to run 'rc --project'
    QProcess rc;
    rc.start(QStringLiteral("rc"), QStringList() << QStringLiteral("--project"));
    // If rc is not installed, return false.
    if (!rc.waitForStarted()) {
        return false;
    }

    // wait for done
    if (!rc.waitForFinished()) {
        return false;
    }

    // if rdm is not running, the exit code is non-zero.
    return (rc.exitStatus() == QProcess::NormalExit) && (rc.exitCode() == 0);
}

bool KateRtagsClient::isIndexing() const
{
    // Try to run 'rc --is-indexing'
    QProcess rc;
    rc.start(QStringLiteral("rc"), QStringList() << QStringLiteral("--is-indexing"));
    // If rc is not installed, return false.
    if (!rc.waitForStarted()) {
        return false;
    }

    // wait for done
    if (!rc.waitForFinished()) {
        return false;
    }

    // get stdout
    const QString output = QString::fromLocal8Bit(rc.readAllStandardOutput());

    // if indexing, "1" is returned. If not indexing, "0" is returned.
    return output.contains(QLatin1Char('1'));
}

QStringList KateRtagsClient::indexedProjects() const
{
    // Try to run 'rc --project'
    QProcess rc;
    rc.start(QStringLiteral("rc"), QStringList() << QStringLiteral("--project"));
    // If rc is not installed, return false.
    if (!rc.waitForStarted()) {
        return QStringList();
    }

    // wait for done
    if (!rc.waitForFinished()) {
        return QStringList();
    }

    // get stdout
    const QString output = QString::fromLocal8Bit(rc.readAllStandardOutput());

    // return projects as string list
    return output.split(QRegularExpression(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);
}

void KateRtagsClient::setCurrentFile(KTextEditor::Document * document)
{
    QUrl url = document->url();
    if (!url.isValid() || !url.isLocalFile()) {
        return;
    }
    
    QString file = url.toLocalFile();

    // run 'rc --current-file <file>'
    QProcess::startDetached(QStringLiteral("rc"), QStringList() << QStringLiteral("--current-file") << file);
}

void KateRtagsClient::followLocation(KTextEditor::Document * document, const KTextEditor::Cursor & cursor)
{
    QUrl url = document->url();
    if (!url.isValid() || !url.isLocalFile()) {
        return;
    }

    const QString location = document->url().toLocalFile() + QStringLiteral(":%1:%2").arg(cursor.line() + 1).arg(cursor.column() + 1);
    qDebug() << "rc command:" << location;

    // run 'rc --follow-location <file>:line:column'
    QProcess rc;
    rc.start(QStringLiteral("rc"), QStringList() << QStringLiteral("--follow-location") << location);
    // If rc is not installed, return false.
    if (!rc.waitForStarted()) {
        return;
    }

    // wait for done
    if (!rc.waitForFinished()) {
        return;
    }

    rc.closeWriteChannel();
    const QString output = QString::fromLocal8Bit(rc.readAllStandardOutput());
    qDebug() << "output" << output;
    QStringList list = output.split(QLatin1Char(':'), QString::SkipEmptyParts);
    qDebug() << list;
    if (list.size() < 3) {
        return;
    }

    // output is: file:line:column
    const QString file = list[0];
    const int line = list[1].toInt() - 1;
    const int column = list[2].toInt() - 1;

    KTextEditor::View * view = KTextEditor::Editor::instance()->application()->activeMainWindow()->openUrl(QUrl::fromLocalFile(file));
    if (!view) {
        return;
    }

    view->setCursorPosition(KTextEditor::Cursor(line, column));
}

#if 0
--> Get uses of symbol
dh@eriador:rtags :-) (master) $ rc --cursorinfo-include-references --cursor-info /home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:87:15
/home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:87:14:          QProcess rc;
SymbolName: QProcess rc
Kind: VarDecl
Type: Record
SymbolLength: 2
Range: 87:5-87:16
Definition
References:
    /home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:88:5:       rc.start(QStringLiteral("rc"), QStringList() << QStringLiteral("--project"));
    /home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:90:10:      if (!rc.waitForStarted()) {
    /home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:95:10:      if (!rc.waitForFinished()) {
    /home/dh/kde/kf5/src/kde/applications/kate/addons/project/rtags/katertagsclient.cpp:100:51:     const QString output = QString::fromLocal8Bit(rc.readAllStandardOutput());

    
    
    
    

dh@eriador:project :-) (master) $ rc --all-references --references /home/dh/kde/kf5/src/kde/applications/kate/addons/project/kateproject.cpp:233:23
/home/dh/kde/kf5/src/kde/applications/kate/addons/project/kateproject.h:171:10:     void saveNotesDocument();
/home/dh/kde/kf5/src/kde/applications/kate/addons/project/kateproject.cpp:47:5:     saveNotesDocument();
/home/dh/kde/kf5/src/kde/applications/kate/addons/project/kateproject.cpp:233:19:       void KateProject::saveNotesDocument()





listtet alle Vorkommen eines Symbols
rc --all-references --references-name KateDocManager



listed alle Funktionsaufrufe
dh@eriador:src :-) (master) $ rc --all-references --references /home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:89:46
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.h:66:28:         KTextEditor::Document *createDoc(const KateDocumentInfo &docInfo = KateDocumentInfo());
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/kateviewspace.cpp:517:70:       KTextEditor::Document *doc = KateApp::self()->documentManager()->createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katemainwindow.cpp:698:49:              KateApp::self()->documentManager()->createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:62:5:        createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:171:15:          doc = createDoc(docInfo);
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:259:9:           createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:453:19:              doc = createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/kateviewmanager.cpp:442:51:         doc = KateApp::self()->documentManager()->createDoc();
/home/dh/kde/kf5/src/kde/applications/kate/kate/src/katedocmanager.cpp:89:40:   KTextEditor::Document *KateDocManager::createDoc(const KateDocumentInfo &docInfo)


#endif
