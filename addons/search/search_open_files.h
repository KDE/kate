/*   Kate search plugin
 *
 * SPDX-FileCopyrightText: 2011-2013 Kåre Särs <kare.sars@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
#include <QTimer>
#include <ktexteditor/document.h>

#include "MatchModel.h"

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
    void matchesFound(const QUrl &url, const QVector<KateSearchMatch> &searchMatches);
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
