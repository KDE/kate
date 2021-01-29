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

#include <QObject>
#include <QStringList>
#include <QRegularExpression>
#include <QRunnable>

#include "MatchModel.h"

class QString;
class QUrl;
class QFile;


class SearchDiskFiles : public QObject, public QRunnable
{
    Q_OBJECT

public:
    SearchDiskFiles(const QStringList &iles, const QRegularExpression &regexp, const bool includeBinaryFiles);

    void run() override;

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void matchesFound(const QUrl &url, const QVector<KateSearchMatch> &searchMatches);

private:
    void searchSingleLineRegExp(QFile &file);
    void searchMultiLineRegExp(QFile &file);

private:
    QStringList m_files;
    QRegularExpression m_regExp;
    bool m_includeBinaryFiles = false;
    bool m_cancelSearch = false;
};

#endif
