/*
    SPDX-FileCopyrightText: 2017 Forrest Smith
    SPDX-FileCopyrightText: 2020 Waqar Ahmed

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KFTS_FUZZY_MATCH_H
#define KFTS_FUZZY_MATCH_H

#include <QString>

/**
 *  This is based on https://github.com/forrestthewoods/lib_fts/blob/master/code/fts_fuzzy_match.h
 *  with modifications for Qt
 */

namespace kfts {
    static bool fuzzy_match_simple(const QChar* pattern, const QChar* str);
    static bool fuzzy_match(const QChar* pattern, const QChar* str, int & outScore);
    static bool fuzzy_match(const QChar* pattern, const QChar* str, int & outScore, uint8_t * matches, int maxMatches);
}

namespace kfts {

    // Forward declarations for "private" implementation
    namespace fuzzy_internal {
        static bool fuzzy_match_recursive(const QChar* pattern, const QChar* str, int & outScore, const QChar* strBegin,
            uint8_t const * srcMatches,  uint8_t * newMatches,  int maxMatches, int nextMatch,
            int & recursionCount, int recursionLimit);
    }

    // Public interface
    static bool fuzzy_match_simple(const QChar* pattern, const QChar* str)
    {
        while (!pattern->isNull() && !str->isNull())  {
            if (pattern->toLower() == str->toLower())
                ++pattern;
            ++str;
        }

        return pattern->isNull() ? true : false;
    }

    static bool fuzzy_match(const QChar* pattern, const QChar* str, int & outScore)
    {
        uint8_t matches[32];
        return fuzzy_match(pattern, str, outScore, matches, sizeof(matches));
    }

    static bool fuzzy_match(const QChar* pattern, const QChar* str, int & outScore, uint8_t * matches, int maxMatches)
    {
        int recursionCount = 0;
        int recursionLimit = 10;

        return fuzzy_internal::fuzzy_match_recursive(pattern, str, outScore, str, nullptr, matches, maxMatches, 0, recursionCount, recursionLimit);
    }

    // Private implementation
    static bool fuzzy_internal::fuzzy_match_recursive(const QChar* pattern,
                                                      const QChar* str,
                                                      int & outScore,
                                                      const QChar* strBegin,
                                                      uint8_t const * srcMatches,
                                                      uint8_t * matches,
                                                      int maxMatches,
                                                      int nextMatch,
                                                      int & recursionCount,
                                                      int recursionLimit)
    {
        // Count recursions
        ++recursionCount;
        if (recursionCount >= recursionLimit)
            return false;

        // Detect end of strings
        if (pattern->isNull() || str->isNull())
            return false;

        // Recursion params
        bool recursiveMatch = false;
        uint8_t bestRecursiveMatches[256];
        int bestRecursiveScore = 0;

        // Loop through pattern and str looking for a match
        bool first_match = true;
        while (!pattern->isNull() && !str->isNull()) {

            // Found match
            if (pattern->toLower() == str->toLower()) {

                // Supplied matches buffer was too short
                if (nextMatch >= maxMatches)
                    return false;

                // "Copy-on-Write" srcMatches into matches
                if (first_match && srcMatches) {
                    memcpy(matches, srcMatches, nextMatch);
                    first_match = false;
                }

                // Recursive call that "skips" this match
                uint8_t recursiveMatches[256];
                int recursiveScore;
                if (fuzzy_match_recursive(pattern, str + 1, recursiveScore, strBegin, matches, recursiveMatches, sizeof(recursiveMatches), nextMatch, recursionCount, recursionLimit)) {

                    // Pick best recursive score
                    if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                        memcpy(bestRecursiveMatches, recursiveMatches, 256);
                        bestRecursiveScore = recursiveScore;
                    }
                    recursiveMatch = true;
                }

                // Advance
                matches[nextMatch++] = (uint8_t)(str - strBegin);
                ++pattern;
            }
            ++str;
        }

        // Determine if full pattern was matched
        bool matched = pattern->isNull() ? true : false;

        // Calculate score
        if (matched) {
            static constexpr int sequential_bonus = 15;            // bonus for adjacent matches
            static constexpr int separator_bonus = 30;             // bonus if match occurs after a separator
            static constexpr int camel_bonus = 30;                 // bonus if match is uppercase and prev is lower
            static constexpr int first_letter_bonus = 15;          // bonus if the first letter is matched

            static constexpr int leading_letter_penalty = -5;      // penalty applied for every letter in str before the first match
            static constexpr int max_leading_letter_penalty = -15; // maximum penalty for leading letters
            static constexpr int unmatched_letter_penalty = -1;    // penalty for every letter that doesn't matter

            // Iterate str to end
            while (!str->isNull())
                ++str;

            // Initialize score
            outScore = 100;

            // Apply leading letter penalty
            int penalty = leading_letter_penalty * matches[0];
            if (penalty < max_leading_letter_penalty)
                penalty = max_leading_letter_penalty;
            outScore += penalty;

            // Apply unmatched penalty
            int unmatched = (int)(str - strBegin) - nextMatch;
            outScore += unmatched_letter_penalty * unmatched;

            // Apply ordering bonuses
            for (int i = 0; i < nextMatch; ++i) {
                uint8_t currIdx = matches[i];

                if (i > 0) {
                    uint8_t prevIdx = matches[i - 1];

                    // Sequential
                    if (currIdx == (prevIdx + 1))
                        outScore += sequential_bonus;
                }

                // Check for bonuses based on neighbor character value
                if (currIdx > 0) {
                    // Camel case
                    QChar neighbor = strBegin[currIdx - 1];
                    QChar curr = strBegin[currIdx];
                    if (neighbor.isLower() && curr.isUpper())
                        outScore += camel_bonus;

                    // Separator
                    bool neighborSeparator = neighbor == QLatin1Char('_') || neighbor == QLatin1Char(' ');
                    if (neighborSeparator)
                        outScore += separator_bonus;
                }
                else {
                    // First letter
                    outScore += first_letter_bonus;
                }
            }
        }

        // Return best result
        if (recursiveMatch && (!matched || bestRecursiveScore > outScore)) {
            // Recursive score is better than "this"
            memcpy(matches, bestRecursiveMatches, maxMatches);
            outScore = bestRecursiveScore;
            return true;
        }
        else if (matched) {
            // "this" score is better than recursive
            return true;
        }
        else {
            // no match
            return false;
        }
    }
} // namespace fts

#endif // KFTS_FUZZY_MATCH_H
