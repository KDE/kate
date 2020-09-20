/*   Kate search plugin
 *
 * SPDX-FileCopyrightText: 2013 Kåre Särs <kare.sars@iki.fi>
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

#ifndef FolderFilesList_h
#define FolderFilesList_h

#include <QElapsedTimer>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QThread>
#include <QVector>

class FolderFilesList : public QThread
{
    Q_OBJECT

public:
    FolderFilesList(QObject *parent = nullptr);
    ~FolderFilesList() override;

    void run() override;

    void generateList(const QString &folder, bool recursive, bool hidden, bool symlinks, bool binary, const QString &types, const QString &excludes);

    void terminateSearch();

    QStringList fileList();

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void searching(const QString &path);
    void fileListReady();

private:
    void checkNextItem(const QFileInfo &item);

private:
    QString m_folder;
    QStringList m_files;
    bool m_cancelSearch = false;

    bool m_recursive = false;
    bool m_hidden = false;
    bool m_symlinks = false;
    bool m_binary = false;
    QStringList m_types;
    QVector<QRegExp> m_excludeList;
    QElapsedTimer m_time;
};

#endif
