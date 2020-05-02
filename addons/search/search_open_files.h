/*   Kate search plugin
 *
 * Copyright (C) 2011-2013 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _SEARCH_OPEN_FILES_H_
#define _SEARCH_OPEN_FILES_H_

#include <QElapsedTimer>
#include <QObject>
#include <QRegularExpression>
#include <ktexteditor/document.h>
#include <QTimer>

class SearchOpenFiles : public QObject
{
    Q_OBJECT

public:
    SearchOpenFiles(QObject *parent = nullptr);

    void startSearch(const QList<KTextEditor::Document *> &list, const QRegularExpression &regexp);
    bool searching();
    void terminateSearch();

public Q_SLOTS:
    void cancelSearch();

    /// return 0 on success or a line number where we stopped.
    int searchOpenFile(KTextEditor::Document *doc, const QRegularExpression &regExp, int startLine);

private Q_SLOTS:
    void doSearchNextFile(int startLine);

private:
    int searchSingleLineRegExp(KTextEditor::Document *doc, const QRegularExpression &regExp, int startLine);
    int searchMultiLineRegExp(KTextEditor::Document *doc, const QRegularExpression &regExp, int startLine);

Q_SIGNALS:
     void matchFound(const QString &url, const QString &fileName, const QString &lineContent, int matchLen, int line, int column, int endLine, int endColumn);
    void searchDone();
    void searching(const QString &file);

private:
    QList<KTextEditor::Document *> m_docList;
    int m_nextFileIndex = -1;
    QTimer m_nextRunTimer;
    int m_nextLine = -1;
    QRegularExpression m_regExp;
    bool m_cancelSearch = true;
    bool m_terminateSearch = false;
    QString m_fullDoc;
    QVector<int> m_lineStart;
    QElapsedTimer m_statusTime;
};

#endif
