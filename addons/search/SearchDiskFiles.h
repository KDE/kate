/*   Kate search plugin
 *
 * SPDX-FileCopyrightText: 2011-2021 Kåre Särs <kare.sars@iki.fi>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

#include "MatchModel.h"

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
    void searchSingleLineRegExp(QFile &file);
    void searchMultiLineRegExp(QFile &file);

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void matchesFound(const QUrl &url, const QVector<KateSearchMatch> &searchMatches);
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
