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

#include <QObject>
#include <QRegularExpression>
#include <QTime>
#include <ktexteditor/document.h>

class SearchOpenFiles: public QObject
{
    Q_OBJECT

public:
    SearchOpenFiles(QObject *parent = nullptr);

    void startSearch(const QList<KTextEditor::Document*> &list,const QRegularExpression &regexp);
    bool searching();

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
    void searchNextFile(int startLine);
    void matchFound(const QString &url, const QString &fileName, int line, int column, const QString &lineContent, int matchLen);
    void searchDone();
    void searching(const QString &file);

private:
    QList<KTextEditor::Document*> m_docList;
    int                           m_nextIndex;
    QRegularExpression            m_regExp;
    bool                          m_cancelSearch;
    QString                       m_fullDoc;
    QVector<int>                  m_lineStart;
    QTime                         m_statusTime;
};


#endif
