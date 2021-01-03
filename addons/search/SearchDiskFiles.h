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

#ifndef SearchDiskFiles_h
#define SearchDiskFiles_h

#include <QElapsedTimer>
#include <QFileInfo>
#include <QMutex>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QVector>

#include <KTextEditor/Range>

/**
 * data holder for one match in one file
 * used to transfer multiple matches at once via signals to avoid heavy costs for files with a lot of matches
 */
class KateSearchMatch
{
    public:
        QString lineContent;
        int matchLen;
        KTextEditor::Range matchRange;
};

Q_DECLARE_METATYPE(KateSearchMatch)

class SearchDiskFiles : public QThread
{
    Q_OBJECT

public:
    SearchDiskFiles(QObject *parent = nullptr);
    ~SearchDiskFiles() override;

    void startSearch(const QStringList &iles, const QRegularExpression &regexp, const bool includeBinaryFiles);
    void run() override;
    void terminateSearch();

    bool searching();

private:
    void searchSingleLineRegExp(const QString &fileName);
    void searchMultiLineRegExp(const QString &fileName);

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void matchesFound(const QString &url, const QString &docName, const QVector<KateSearchMatch> &searchMatches);
    void searchDone();
    void searching(const QString &file);

private:
    QRegularExpression m_regExp;
    QStringList m_files;
    bool m_cancelSearch = true;
    bool m_terminateSearch = false;
    int m_matchCount = 0;
    QElapsedTimer m_statusTime;
    bool m_includeBinaryFiles = false;
};

#endif
