/*
    SPDX-FileCopyrightText: 2025 Kåre Särs <kare.sars@iki.fi>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitblameparser.h"

#include <KLocalizedString>

#include <QDateTime>

KateGitBlameParser::KateGitBlameParser()
{
}

struct LineBlock {
    int commitEnd;
    int lineEnd;
};

static LineBlock nextBlock(QByteArrayView rawData, int from)
{
    // A line block is either
    // <40 char hash> 1 2 3\n
    // \t<line data>\n
    // or
    // <40 char hash> 1 2 3\n
    // author ...\n
    // ...
    // author-time ...\n
    // ...
    // summary ...
    // \t<line data>\n
    //
    // As every code line starts with a \t, we can use \n\t to find the beginning of the code line
    int commitEnd = rawData.indexOf("\n\t", from) + 1;
    if (commitEnd == 0) {
        return {-1, -1};
    }
    int blockEnd = rawData.indexOf('\n', commitEnd);
    return {commitEnd, blockEnd};
}

bool KateGitBlameParser::parseGitBlame(const QByteArray &blame)
{
    // Save the data, everything else has views over this data
    m_rawCommitData = blame;
    m_rawCommitData.replace("\r", ""); // KTextEditor removes all \r characters in the internal buffers
    QByteArrayView out(m_rawCommitData);

    QHash<QByteArrayView, QString> authorCache;

    /**
     * The output contains info about each line of text and commit info
     * for that line. We store the commit info separately in a hash-map
     * so that they don't need to be duplicated. For each line we store
     * its line text and short commit. Text is needed because if you
     * modify the doc, we use it to figure out where the original blame
     * line is. The short commit is used to fetch the full commit from
     * the hashmap
     */

    int start = 0;

    while (start != -1) {
        auto block = nextBlock(out, start);
        if (block.commitEnd == -1 || block.lineEnd == -1) {
            return true;
        }
        // qDebug() << start << block.commitEnd << block.lineEnd;
        // qDebug() << out.mid(start, block.commitEnd - start);
        // qDebug() << out.mid(block.commitEnd, block.lineEnd - block.commitEnd);

        CommitInfo commitInfo;
        BlamedLine lineInfo;

        /**
         * Parse hash and line numbers
         *
         * 5c7f27a0915a9b20dc9f683d0d85b6e4b829bc85 1 1 5
         */
        int hashEnd = out.indexOf(' ', start);
        constexpr int hashLen = 40;
        if (hashEnd == -1 || (hashEnd - start) != hashLen) {
            qWarning() << "Not a proper hash:" << out.mid(start, hashEnd - start);
            return false;
        }
        QByteArrayView hash = out.mid(start, hashEnd - start);
        lineInfo.shortCommitHash = hash.mid(0, 7);
        m_blameLines.push_back(lineInfo);

        // skip to line end,
        // we don't care about line no etc here
        start = out.indexOf('\n', hashEnd);
        if (start == -1) {
            qWarning("Git blame: Invalid blame output : No new line");
            return false;
        }
        start++;

        // are we done because this line references the commit instead of
        // containing the content?
        if (start == block.commitEnd) {
            start++; // skip the \t
            m_blameLines.back().lineText = out.mid(start, block.lineEnd - start);
            start = block.lineEnd + 1;
            continue;
        }

        /**
         * Parse commit
         */
        commitInfo.hash = hash;

        // author Xyz
        constexpr int authorLen = (int)sizeof("author");
        start += authorLen;
        int authorEnd = out.indexOf('\n', start);

        QByteArrayView authorView = out.mid(start, authorEnd - start);
        auto it = authorCache.find(authorView);
        if (it == authorCache.end()) {
            it = authorCache.insert(authorView, QString::fromUtf8(authorView));
        }
        commitInfo.authorName = it.value();

        start++;

        // author-time timestamp
        constexpr int authorTimeLen = (int)sizeof("author-time");
        start = out.indexOf("author-time ", start);
        if (start == -1) {
            qWarning("Invalid commit while git-blameing");
            return false;
        }
        start += authorTimeLen;
        int timeEnd = out.indexOf('\n', start);

        qint64 timestamp = out.mid(start, timeEnd - start).toLongLong();
        commitInfo.authorDate = QDateTime::fromSecsSinceEpoch(timestamp);

        constexpr int summaryLen = (int)sizeof("summary");
        start = out.indexOf("summary ", start);
        start += summaryLen;
        int summaryEnd = out.indexOf('\n', start);

        commitInfo.summary = out.mid(start, summaryEnd - start);
        // qDebug() << "\nCommit {\n" //
        //          << commitInfo.hash << "\n" //
        //          << commitInfo.authorName << "\n" //
        //          << commitInfo.authorDate.toString() << "\n" //
        //          << commitInfo.summary << "\n}";

        m_blameInfoForHash[lineInfo.shortCommitHash] = commitInfo;
        m_blameLines.back().lineText = out.mid(block.commitEnd + 1, block.lineEnd - block.commitEnd - 1);
        start = block.lineEnd + 1;
    }

    return true;
}

bool KateGitBlameParser::hasBlameInfo() const
{
    return m_blameLines.size() != 0 && !m_blameInfoForHash.isEmpty();
}

int KateGitBlameParser::blameLineCount() const
{
    return (int)m_blameLines.size();
}

const QByteArrayView &KateGitBlameParser::blameLine(int lineNr)
{
    static QByteArrayView dummy;
    if (m_blameLines.empty() || lineNr < 0 || lineNr >= (int)m_blameLines.size()) {
        return dummy;
    }
    return m_blameLines[lineNr].lineText;
}

const CommitInfo &KateGitBlameParser::blameLineInfo(int lineNr)
{
    static const CommitInfo dummy{.hash = "hash", //
                                  .authorName = i18n("Not Committed Yet"),
                                  .authorDate = QDateTime::currentDateTime(),
                                  .summary = {}};
    if (m_blameLines.empty() || lineNr < 0 || lineNr >= (int)m_blameLines.size()) {
        return dummy;
    }

    auto &commitInfo = m_blameLines[lineNr];

    Q_ASSERT(m_blameInfoForHash.contains(commitInfo.shortCommitHash));
    return m_blameInfoForHash[commitInfo.shortCommitHash];
}

void KateGitBlameParser::clear()
{
    m_rawCommitData.clear();
    m_blameLines.clear();
    m_blameInfoForHash.clear();
}
