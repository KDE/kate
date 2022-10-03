/*
    SPDX-FileCopyrightText: 2017 Forrest Smith
    SPDX-FileCopyrightText: 2020 Waqar Ahmed

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KFTS_FUZZY_MATCH_H
#define KFTS_FUZZY_MATCH_H

#include <QDebug>
#include <QString>
#include <QTextLayout>

/**
 * This is based on https://github.com/forrestthewoods/lib_fts/blob/master/code/fts_fuzzy_match.h
 * with modifications for Qt
 *
 * Dont include this file in a header file, please :)
 */

namespace kfts
{
/**
 * @brief simple fuzzy matching of chars in @a pattern with chars in @a str sequentially
 */
Q_DECL_UNUSED static bool fuzzy_match_simple(const QStringView pattern, const QStringView str);

/**
 * @brief This should be the main function you should use. @a outscore is the score
 * of this match and should be used to sort the results later. Without sorting of the
 * results this function won't be as effective.
 */
Q_DECL_UNUSED static bool fuzzy_match(const QStringView pattern, const QStringView str, int &outScore);
Q_DECL_UNUSED static bool fuzzy_match(const QStringView pattern, const QStringView str, int &outScore, uint8_t *matches);

/**
 * @brief get string for display in treeview / listview. This should be used from style delegate.
 * For example: with @a pattern = "kate", @a str = "kateapp" and @htmlTag = "<b>
 * the output will be <b>k</b><b>a</b><b>t</b><b>e</b>app which will be visible to user as
 * <b>kate</b>app.
 *
 * TODO: improve this so that we don't have to put html tags on every char probably using some kind
 * of interval container
 */
Q_DECL_UNUSED static QString to_fuzzy_matched_display_string(const QStringView pattern, QString &str, const QString &htmlTag, const QString &htmlTagClose);
Q_DECL_UNUSED static QString
to_scored_fuzzy_matched_display_string(const QStringView pattern, QString &str, const QString &htmlTag, const QString &htmlTagClose);
}

namespace kfts
{
// Forward declarations for "private" implementation
namespace fuzzy_internal
{
static inline constexpr QChar toLower(QChar c)
{
    return c.isLower() ? c : c.toLower();
}

static bool fuzzy_match_recursive(QStringView::const_iterator pattern,
                                  QStringView::const_iterator str,
                                  int &outScore,
                                  const QStringView::const_iterator strBegin,
                                  const QStringView::const_iterator strEnd,
                                  const QStringView::const_iterator patternEnd,
                                  uint8_t const *srcMatches,
                                  uint8_t *newMatches,
                                  int nextMatch,
                                  int &totalMatches,
                                  int &recursionCount);
}

// Public interface
static bool fuzzy_match_simple(const QStringView pattern, const QStringView str)
{
    if (pattern.length() == 1) {
        return str.contains(pattern, Qt::CaseInsensitive);
    }

    auto patternIt = pattern.cbegin();
    for (auto strIt = str.cbegin(); strIt != str.cend() && patternIt != pattern.cend(); ++strIt) {
        if (strIt->toLower() == patternIt->toLower()) {
            ++patternIt;
        }
    }
    return patternIt == pattern.cend();
}

static bool fuzzy_match(const QStringView pattern, const QStringView str, int &outScore)
{
    if (pattern.length() == 1) {
        const int found = str.indexOf(pattern, Qt::CaseInsensitive);
        if (found >= 0) {
            outScore = 250 - found;
            outScore += pattern.at(0) == str.at(found);
            return true;
        } else {
            outScore = 0;
            return false;
        }
    }

    // simple substring matching to flush out non-matching stuff
    auto patternIt = pattern.cbegin();
    bool lower = patternIt->isLower();
    QChar cUp = lower ? patternIt->toUpper() : *patternIt;
    QChar cLow = lower ? *patternIt : patternIt->toLower();
    for (auto strIt = str.cbegin(); strIt != str.cend() && patternIt != pattern.cend(); ++strIt) {
        if (*strIt == cLow || *strIt == cUp) {
            ++patternIt;
            lower = patternIt->isLower();
            cUp = lower ? patternIt->toUpper() : *patternIt;
            cLow = lower ? *patternIt : patternIt->toLower();
        }
    }

    if (patternIt != pattern.cend()) {
        outScore = 0;
        return false;
    }

    uint8_t matches[256];
    return fuzzy_match(pattern, str, outScore, matches);
}

static bool fuzzy_match(const QStringView pattern, const QStringView str, int &outScore, uint8_t *matches)
{
    int recursionCount = 0;

    auto strIt = str.cbegin();
    auto patternIt = pattern.cbegin();
    const auto patternEnd = pattern.cend();
    const auto strEnd = str.cend();
    int totalMatches = 0;

    return fuzzy_internal::fuzzy_match_recursive(patternIt, strIt, outScore, strIt, strEnd, patternEnd, nullptr, matches, 0, totalMatches, recursionCount);
}

// Private implementation
static bool fuzzy_internal::fuzzy_match_recursive(QStringView::const_iterator pattern,
                                                  QStringView::const_iterator str,
                                                  int &outScore,
                                                  const QStringView::const_iterator strBegin,
                                                  const QStringView::const_iterator strEnd,
                                                  const QStringView::const_iterator patternEnd,
                                                  const uint8_t *srcMatches,
                                                  uint8_t *matches,
                                                  int nextMatch,
                                                  int &totalMatches,
                                                  int &recursionCount)
{
    static constexpr int recursionLimit = 10;
    // max number of matches allowed, this should be enough
    static constexpr int maxMatches = 256;

    // Count recursions
    ++recursionCount;
    if (recursionCount >= recursionLimit) {
        return false;
    }

    // Detect end of strings
    if (pattern == patternEnd || str == strEnd) {
        return false;
    }

    // Recursion params
    bool recursiveMatch = false;
    uint8_t bestRecursiveMatches[maxMatches];
    int bestRecursiveScore = 0;

    // Loop through pattern and str looking for a match
    bool firstMatch = true;
    QChar currentPatternChar = toLower(*pattern);
    // Are we matching in sequence start from start?
    while (pattern != patternEnd && str != strEnd) {
        // Found match
        if (currentPatternChar == toLower(*str)) {
            // Supplied matches buffer was too short
            if (nextMatch >= maxMatches) {
                return false;
            }

            // "Copy-on-Write" srcMatches into matches
            if (firstMatch && srcMatches) {
                memcpy(matches, srcMatches, nextMatch);
                firstMatch = false;
            }

            // Recursive call that "skips" this match
            uint8_t recursiveMatches[maxMatches];
            int recursiveScore = 0;
            const auto strNextChar = std::next(str);
            if (fuzzy_match_recursive(pattern,
                                      strNextChar,
                                      recursiveScore,
                                      strBegin,
                                      strEnd,
                                      patternEnd,
                                      matches,
                                      recursiveMatches,
                                      nextMatch,
                                      totalMatches,
                                      recursionCount)) {
                // Pick best recursive score
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, maxMatches);
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = true;
            }

            // Advance
            matches[nextMatch++] = (uint8_t)(std::distance(strBegin, str));
            ++pattern;
            currentPatternChar = toLower(*pattern);
        }
        ++str;
    }

    // Determine if full pattern was matched
    const bool matched = pattern == patternEnd;

    // Calculate score
    if (matched) {
        static constexpr int firstSepScoreDiff = 3;

        static constexpr int sequentialBonus = 20;
        static constexpr int separatorBonus = 25; // bonus if match occurs after a separator
        static constexpr int firstLetterBonus = 15; // bonus if the first letter is matched
        static constexpr int firstLetterSepMatchBonus = firstLetterBonus - firstSepScoreDiff; // bonus if the first matched letter is camel or separator

        static constexpr int unmatchedLetterPenalty = -1; // penalty for every letter that doesn't matter

        int nonBeginSequenceBonus = 10;
        // points by which nonBeginSequenceBonus is increment on every matched letter
        static constexpr int nonBeginSequenceIncrement = 4;

        // Initialize score
        outScore = 100;

#define debug_algo 0
#if debug_algo
#define dbg(...) qDebug(__VA_ARGS__)
#else
#define dbg(...)
#endif

        // Apply unmatched penalty
        const int unmatched = (int)(std::distance(strBegin, strEnd)) - nextMatch;
        outScore += unmatchedLetterPenalty * unmatched;
        dbg("unmatchedLetterPenalty, unmatched count: %d, outScore: %d", unmatched, outScore);

        bool inSeparatorSeq = false;
        int i = 0;
        if (matches[i] == 0) {
            // First letter match has the highest score
            outScore += firstLetterBonus + separatorBonus;
            dbg("firstLetterBonus, outScore: %d", outScore);
            inSeparatorSeq = true;
        } else {
            const QChar neighbor = *(strBegin + matches[i] - 1);
            const QChar curr = *(strBegin + matches[i]);
            const bool neighborSeparator = neighbor == QLatin1Char('_') || neighbor == QLatin1Char(' ');
            if (neighborSeparator || (neighbor.isLower() && curr.isUpper())) {
                // the first letter that got matched was a sepcial char .e., camel or at a separator
                outScore += firstLetterSepMatchBonus + separatorBonus;
                dbg("firstLetterSepMatchBonus at %d, letter: %c, outScore: %d", matches[i], curr.toLatin1(), outScore);
                inSeparatorSeq = true;
            } else {
                // nothing
                nonBeginSequenceBonus += nonBeginSequenceIncrement;
            }
            // We didn't match any special positions, apply leading penalty
            outScore += -(matches[i]);
            dbg("LeadingPenalty because no first letter match, outScore: %d", outScore);
        }
        i++;

        bool allConsecutive = true;
        // Apply ordering bonuses
        for (; i < nextMatch; ++i) {
            const uint8_t currIdx = matches[i];
            const uint8_t prevIdx = matches[i - 1];
            // Sequential
            if (currIdx == (prevIdx + 1)) {
                if (i == matches[i]) {
                    // We are in a sequence beginning from first letter
                    outScore += sequentialBonus;
                    dbg("sequentialBonus at %d, letter: %c, outScore: %d", matches[i], (strBegin + currIdx)->toLatin1(), outScore);
                } else if (inSeparatorSeq) {
                    // we are in a sequnce beginning from a separator like camelHump or underscore
                    outScore += sequentialBonus - firstSepScoreDiff;
                    dbg("in separator seq, [sequentialBonus - 5] at %d, letter: %c, outScore: %d", matches[i], (strBegin + currIdx)->toLatin1(), outScore);
                } else {
                    // We are in a random sequence
                    outScore += nonBeginSequenceBonus;
                    nonBeginSequenceBonus += nonBeginSequenceIncrement;
                    dbg("nonBeginSequenceBonus at %d, letter: %c, outScore: %d", matches[i], (strBegin + currIdx)->toLatin1(), outScore);
                }
            } else {
                allConsecutive = false;

                // there is a gap between matching chars, apply penalty
                int penalty = -((currIdx - prevIdx)) - 2;
                outScore += penalty;
                inSeparatorSeq = false;
                nonBeginSequenceBonus = 10;
                dbg("gap penalty[%d] at %d, letter: %c, outScore: %d", penalty, matches[i], (strBegin + currIdx)->toLatin1(), outScore);
            }

            // Check for bonuses based on neighbor character value
            // Camel case
            const QChar neighbor = *(strBegin + currIdx - 1);
            const QChar curr = *(strBegin + currIdx);
            // if camel case bonus, then not snake / separator.
            // This prevents double bonuses
            const bool neighborSeparator = neighbor == QLatin1Char('_') || neighbor == QLatin1Char(' ');
            if (neighborSeparator || (neighbor.isLower() && curr.isUpper())) {
                outScore += separatorBonus;
                dbg("separatorBonus at %d, letter: %c, outScore: %d", matches[i], (strBegin + currIdx)->toLatin1(), outScore);
                inSeparatorSeq = true;
                continue;
            }
        }

        if (allConsecutive && nextMatch >= 4) {
            outScore *= 2;
            dbg("allConsecutive double the score, outScore: %d", outScore);
        }
    }

    totalMatches = nextMatch;
    // Return best result
    if (recursiveMatch && (!matched || bestRecursiveScore > outScore)) {
        // Recursive score is better than "this"
        memcpy(matches, bestRecursiveMatches, maxMatches);
        outScore = bestRecursiveScore;
        return true;
    } else if (matched) {
        // "this" score is better than recursive
        return true;
    } else {
        // no match
        return false;
    }
}

static QString to_fuzzy_matched_display_string(const QStringView pattern, QString &str, const QString &htmlTag, const QString &htmlTagClose)
{
    /**
     * FIXME Don't do so many appends. Instead consider using some interval based solution to wrap a range
     * of text with the html <tag></tag>
     */
    int j = 0;
    for (int i = 0; i < str.size() && j < pattern.size(); ++i) {
        if (fuzzy_internal::toLower(str.at(i)) == fuzzy_internal::toLower(pattern.at(j))) {
            str.replace(i, 1, htmlTag + str.at(i) + htmlTagClose);
            i += htmlTag.size() + htmlTagClose.size();
            ++j;
        }
    }
    return str;
}

static QString to_scored_fuzzy_matched_display_string(const QStringView pattern, QString &str, const QString &htmlTag, const QString &htmlTagClose)
{
    if (pattern.isEmpty()) {
        return str;
    }

    uint8_t matches[256];
    int totalMatches = 0;
    {
        int score = 0;
        int recursionCount = 0;

        auto strIt = str.cbegin();
        auto patternIt = pattern.cbegin();
        const auto patternEnd = pattern.cend();
        const auto strEnd = str.cend();

        fuzzy_internal::fuzzy_match_recursive(patternIt, strIt, score, strIt, strEnd, patternEnd, nullptr, matches, 0, totalMatches, recursionCount);
    }

    int offset = 0;
    for (int i = 0; i < totalMatches; ++i) {
        str.insert(matches[i] + offset, htmlTag);
        offset += htmlTag.size();
        str.insert(matches[i] + offset + 1, htmlTagClose);
        offset += htmlTagClose.size();
    }

    return str;
}

Q_DECL_UNUSED static QVector<QTextLayout::FormatRange>
get_fuzzy_match_formats(const QStringView pattern, const QStringView str, int offset, const QTextCharFormat &fmt)
{
    QVector<QTextLayout::FormatRange> ranges;
    if (pattern.isEmpty()) {
        return ranges;
    }

    int totalMatches = 0;
    int score = 0;
    int recursionCount = 0;

    auto strIt = str.cbegin();
    auto patternIt = pattern.cbegin();
    const auto patternEnd = pattern.cend();
    const auto strEnd = str.cend();

    uint8_t matches[256];
    fuzzy_internal::fuzzy_match_recursive(patternIt, strIt, score, strIt, strEnd, patternEnd, nullptr, matches, 0, totalMatches, recursionCount);

    int j = 0;
    for (int i = 0; i < totalMatches; ++i) {
        auto matchPos = matches[i];
        if (!ranges.isEmpty() && matchPos == j + 1) {
            ranges.last().length++;
        } else {
            ranges.append({matchPos + offset, 1, fmt});
        }
        j = matchPos;
    }

    return ranges;
}

} // namespace kfts

#endif // KFTS_FUZZY_MATCH_H
