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

SearchDiskFiles::SearchDiskFiles(QObject *parent)
    : QThread(parent)
{
    // ensure we have a proper thread name during e.g. perf profiling
    setObjectName(QStringLiteral("SearchDiskFiles"));
}

SearchDiskFiles::~SearchDiskFiles()
{
    m_cancelSearch = true;
    wait();
}

void SearchDiskFiles::startSearch(const QStringList &files, const QRegularExpression &regexp, const bool includeBinaryFiles)
{
    if (files.empty()) {
        emit searchDone();
        return;
    }
    m_includeBinaryFiles = includeBinaryFiles;
    m_cancelSearch = false;
    m_terminateSearch = false;
    m_files = files;
    m_regExp = regexp;
    m_matchCount = 0;
    m_statusTime.restart();
    start();
}

void SearchDiskFiles::run()
{
    for (const QString &fileName : qAsConst(m_files)) {
        if (m_cancelSearch) {
            break;
        }

        if (m_statusTime.elapsed() > 100) {
            m_statusTime.restart();
            emit searching(fileName);
        }

        // exclude binary files?
        if (!m_includeBinaryFiles) {
            const auto mimeType = QMimeDatabase().mimeTypeForFile(fileName);
            if (!mimeType.inherits(QStringLiteral("text/plain"))) {
                continue;
            }
        }

        if (m_regExp.pattern().contains(QLatin1String("\\n"))) {
            searchMultiLineRegExp(fileName);
        } else {
            searchSingleLineRegExp(fileName);
        }
    }

    if (!m_terminateSearch) {
        emit searchDone();
    }
    m_cancelSearch = true;
}

void SearchDiskFiles::cancelSearch()
{
    m_cancelSearch = true;
}

void SearchDiskFiles::terminateSearch()
{
    m_cancelSearch = true;
    m_terminateSearch = true;
    wait();
}

bool SearchDiskFiles::searching()
{
    return !m_cancelSearch;
}

void SearchDiskFiles::searchSingleLineRegExp(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        return;
    }

    QTextStream stream(&file);
    QString line;
    int i = 0;
    int column;
    QRegularExpressionMatch match;
    QVector<KateSearchMatch> matches;
    while (!(line = stream.readLine()).isNull()) {
        if (m_cancelSearch)
            break;
        match = m_regExp.match(line);
        column = match.capturedStart();
        while (column != -1 && !match.captured().isEmpty()) {
            if (m_cancelSearch)
                break;

            int endColumn = column + match.capturedLength();
            int preContextStart = qMax(0, column-MatchModel::PreContextLen);
            QString preContext = line.mid(preContextStart, column-preContextStart);
            QString postContext = line.mid(endColumn, MatchModel::PostContextLen);

            matches.push_back(KateSearchMatch{preContext, match.captured(), postContext, QString(), KTextEditor::Range{i, column, i, column + match.capturedLength()}, true});

            match = m_regExp.match(line, column + match.capturedLength());
            column = match.capturedStart();
            m_matchCount++;
            // NOTE: This sleep is here so that the main thread will get a chance to
            // handle any stop button clicks if there are a lot of matches
            if (m_matchCount % 50)
                msleep(1);
        }
        i++;
    }

    // emit all matches batched
    const QUrl fileUrl = QUrl::fromUserInput(fileName);
    emit matchesFound(fileUrl, matches);
}

void SearchDiskFiles::searchMultiLineRegExp(const QString &fileName)
{
    QFile file(fileName);
    int column = 0;
    int line = 0;
    static QString fullDoc;
    static QVector<int> lineStart;
    QRegularExpression tmpRegExp = m_regExp;

    if (!file.open(QFile::ReadOnly)) {
        return;
    }

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
        if (m_cancelSearch)
            break;
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

        int preContextStart = qMax(lineStart[line], column-MatchModel::PreContextLen);
        QString preContext = fullDoc.mid(preContextStart, column-preContextStart);
        QString postContext = fullDoc.mid(column + match.captured().length(), MatchModel::PostContextLen);

        matches.push_back(KateSearchMatch{preContext, match.captured(), postContext, QString(), KTextEditor::Range{line, startColumn, endLine, endColumn}, true});

        match = tmpRegExp.match(fullDoc, column + match.capturedLength());
        column = match.capturedStart();
        m_matchCount++;
        // NOTE: This sleep is here so that the main thread will get a chance to
        // handle any stop button clicks if there are a lot of matches
        if (m_matchCount % 50)
            msleep(1);
    }

    // emit all matches batched
    const QUrl fileUrl = QUrl::fromUserInput(fileName);
    emit matchesFound(fileUrl, matches);
}
