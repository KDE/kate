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
#include <QElapsedTimer>
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
    // do we need to search multiple lines?
    const bool multiLineSearch = m_regExp.pattern().contains(QLatin1String("\\n"));

    // timer to emit matchesFound once in a time even for files without matches
    // this triggers process in the UI
    QElapsedTimer emitTimer;
    emitTimer.start();

    // search, pulls work from the shared work list for all workers
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

        // let the right search algorithm compute the matches for this file
        QVector<KateSearchMatch> matches;
        if (multiLineSearch) {
            matches = searchMultiLineRegExp(file);
        } else {
            matches = searchSingleLineRegExp(file);
        }

        // if we have matches or didn't emit something long enough, do so
        // we don't emit for all file to not stall get GUI and lock us a lot ;)
        if (!matches.isEmpty() || emitTimer.hasExpired(100)) {
            Q_EMIT matchesFound(QUrl::fromLocalFile(file.fileName()), matches);
            emitTimer.restart();
        }
    }
}

QVector<KateSearchMatch> SearchDiskFiles::searchSingleLineRegExp(QFile &file)
{
    QTextStream stream(&file);
    int i = 0;
    int column;
    QRegularExpressionMatch match;
    QVector<KateSearchMatch> matches;
    QString line;
    while (stream.readLineInto(&line)) {
        if (m_worklist.isCanceled()) {
            break;
        }

        // check if not binary data....
        // bad, but stuff better than asking QMimeDatabase which is a performance & threading disaster...
        if (!m_includeBinaryFiles && line.contains(QLatin1Char('\0'))) {
            // kill all seen matches and be done
            matches.clear();
            return matches;
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
    return matches;
}

QVector<KateSearchMatch> SearchDiskFiles::searchMultiLineRegExp(QFile &file)
{
    int column = 0;
    int line = 0;
    static QString fullDoc;
    static QVector<int> lineStart;
    QRegularExpression tmpRegExp = m_regExp;

    QVector<KateSearchMatch> matches;
    QTextStream stream(&file);
    fullDoc = stream.readAll();

    // check if not binary data....
    // bad, but stuff better than asking QMimeDatabase which is a performance & threading disaster...
    if (!m_includeBinaryFiles && fullDoc.contains(QLatin1Char('\0'))) {
        // kill all seen matches and be done
        matches.clear();
        return matches;
    }

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
    return matches;
}
