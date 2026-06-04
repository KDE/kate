/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QString>

/// Zero-alloc keyword matcher for SQL tokens.  Stores up to 8 ASCII
/// chars on the stack and can test for "SELECT" / "EXPLAIN" without
/// any heap allocation.
class SQLQueryKeywordMatcher
{
public:
    /// First keyword detected at the start of a statement.
    enum class FirstKeyword {
        None,
        Select,
        Explain,
        Other,
    };

    void feed(QChar c, bool isLetter)
    {
        if (done)
            return;
        if (!isLetter) {
            done = true;
            return;
        }
        if (len < 8)
            buf[len++] = c.toUpper().toLatin1();
    }

    /// Mark the current keyword as finished when whitespace is encountered.
    void spaceTerminated()
    {
        if (!done && len > 0)
            done = true;
    }

    /// Force the matcher into the finished state without recording a char.
    /// Used e.g. when an opening '(' is encountered — no keyword is
    /// expected between the '(' and whatever follows.
    void forceFinish()
    {
        done = true;
    }

    bool isDone() const
    {
        return done;
    }

    bool isSelect() const
    {
        return done && len == 6 && qstrncmp(buf, "SELECT", 6) == 0;
    }

    bool isExplain() const
    {
        return done && len == 7 && qstrncmp(buf, "EXPLAIN", 7) == 0;
    }

    /// Convert the captured keyword into a FirstKeyword enum value.
    FirstKeyword toFirstKeyword() const
    {
        if (isSelect())
            return FirstKeyword::Select;
        if (isExplain())
            return FirstKeyword::Explain;
        if (done)
            return FirstKeyword::Other;
        return FirstKeyword::None;
    }

    void reset()
    {
        len = 0;
        done = false;
    }

private:
    char buf[8] = {};
    int len = 0;
    bool done = false;
};
