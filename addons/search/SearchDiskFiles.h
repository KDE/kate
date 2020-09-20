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

class SearchDiskFiles : public QThread
{
    Q_OBJECT

public:
    SearchDiskFiles(QObject *parent = nullptr);
    ~SearchDiskFiles() override;

    void startSearch(const QStringList &iles, const QRegularExpression &regexp);
    void run() override;
    void terminateSearch();

    bool searching();

private:
    void searchSingleLineRegExp(const QString &fileName);
    void searchMultiLineRegExp(const QString &fileName);

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void matchFound(const QString &url, const QString &docName, const QString &lineContent, int matchLen, int line, int column, int endLine, int endColumn);
    void searchDone();
    void searching(const QString &file);

private:
    QRegularExpression m_regExp;
    QStringList m_files;
    bool m_cancelSearch = true;
    bool m_terminateSearch = false;
    int m_matchCount = 0;
    QElapsedTimer m_statusTime;
};

#endif
