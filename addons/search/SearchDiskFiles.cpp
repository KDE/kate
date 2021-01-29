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

#include "SearchDiskFiles.h"

#include <QDir>
#include <QMimeDatabase>
#include <QTextStream>
#include <QUrl>

SearchDiskFiles::SearchDiskFiles(SearchDiskFilesWorkList &worklist, const QRegularExpression &regexp, const bool includeBinaryFiles)
    : m_worklist(worklist)
    , m_regExp(regexp.pattern(), regexp.patternOptions()) // we WANT to kill the sharing, ELSE WE LOCK US DEAD!
    , m_includeBinaryFiles(includeBinaryFiles)
{
    // ensure we have a proper thread name during e.g. perf profiling
    setObjectName(QStringLiteral("SearchDiskFiles"));
}

void SearchDiskFiles::run()
{
    const QMimeDatabase db;
    while (true) {
        // get next file, we get empty string if all done or search canceled!
        const auto fileName = m_worklist.nextFileToSearch();
        if (fileName.isEmpty()) {
            break;
        }

        // open file early, this allows mime-type detection & search to use same io device
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            continue;
        }

        // exclude binary files?
        if (!m_includeBinaryFiles) {
            /*   const auto mimeType = db.mimeTypeForFileNameAndData(fileName, &file);
               if (!mimeType.inherits(QStringLiteral("text/plain"))) {
                   continue;
               }*/
        }

        if (m_regExp.pattern().contains(QLatin1String("\\n"))) {
            searchMultiLineRegExp(file);
        } else {
            searchSingleLineRegExp(file);
        }
    }
}

void SearchDiskFiles::searchSingleLineRegExp(QFile &file)
{
    QTextStream stream(&file);
    QString line;
    int i = 0;
    int column;
    QRegularExpressionMatch match;
    QVector<KateSearchMatch> matches;
    while (!(line = stream.readLine()).isNull()) {
        if (m_worklist.isCanceled()) {
            break;
        }
        match = m_regExp.match(line);
        column = match.capturedStart();
        while (column != -1 && !match.captured().isEmpty()) {
            if (m_worklist.isCanceled()) {
                break;
            }

            int endColumn = column + match.capturedLength();
            int preContextStart = qMax(0, column - MatchModel::PreContextLen);
            QString preContext = line.mid(preContextStart, column - preContextStart);
            QString postContext = line.mid(endColumn, MatchModel::PostContextLen);

            matches.push_back(
                KateSearchMatch{preContext, match.captured(), postContext, QString(), KTextEditor::Range{i, column, i, column + match.capturedLength()}, true});

            match = m_regExp.match(line, column + match.capturedLength());
            column = match.capturedStart();
        }
        i++;
    }

    // Q_EMIT all matches batched
    const QUrl fileUrl = QUrl::fromUserInput(file.fileName());
    Q_EMIT matchesFound(fileUrl, matches);
}

void SearchDiskFiles::searchMultiLineRegExp(QFile &file)
{
    int column = 0;
    int line = 0;
    static QString fullDoc;
    static QVector<int> lineStart;
    QRegularExpression tmpRegExp = m_regExp;

    QTextStream stream(&file);
    fullDoc = stream.readAll();
    fullDoc.remove(QLatin1Char('\r'));

    lineStart.clear();
    lineStart << 0;
    for (int i = 0; i < fullDoc.size() - 1; i++) {
        if (fullDoc[i] == QLatin1Char('\n')) {
            lineStart << i + 1;
        }
    }
    if (tmpRegExp.pattern().endsWith(QLatin1Char('$'))) {
        fullDoc += QLatin1Char('\n');
        QString newPatern = tmpRegExp.pattern();
        newPatern.replace(QStringLiteral("$"), QStringLiteral("(?=\\n)"));
        tmpRegExp.setPattern(newPatern);
    }

    QRegularExpressionMatch match;
    match = tmpRegExp.match(fullDoc);
    column = match.capturedStart();
    QVector<KateSearchMatch> matches;
    while (column != -1 && !match.captured().isEmpty()) {
        if (m_worklist.isCanceled()) {
            break;
        }
        // search for the line number of the match
        int i;
        line = -1;
        for (i = 1; i < lineStart.size(); i++) {
            if (lineStart[i] > column) {
                line = i - 1;
                break;
            }
        }
        if (line == -1) {
            break;
        }
        int startColumn = (column - lineStart[line]);
        int endLine = line + match.captured().count(QLatin1Char('\n'));
        int lastNL = match.captured().lastIndexOf(QLatin1Char('\n'));
        int endColumn = lastNL == -1 ? startColumn + match.captured().length() : match.captured().length() - lastNL - 1;

        int preContextStart = qMax(lineStart[line], column - MatchModel::PreContextLen);
        QString preContext = fullDoc.mid(preContextStart, column - preContextStart);
        QString postContext = fullDoc.mid(column + match.captured().length(), MatchModel::PostContextLen);

        matches.push_back(
            KateSearchMatch{preContext, match.captured(), postContext, QString(), KTextEditor::Range{line, startColumn, endLine, endColumn}, true});

        match = tmpRegExp.match(fullDoc, column + match.capturedLength());
        column = match.capturedStart();
    }

    // Q_EMIT all matches batched
    const QUrl fileUrl = QUrl::fromUserInput(file.fileName());
    Q_EMIT matchesFound(fileUrl, matches);
}
