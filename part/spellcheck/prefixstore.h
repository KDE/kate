/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 by Michel Ludwig <michel.ludwig@kdemail.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef PREFIXSTORE_H
#define PREFIXSTORE_H

#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVector>

#include "katetextline.h"

/**
 * This class can be used to efficiently search for occurrences of strings in
 * a given string. Theoretically speaking, a finite deterministic automaton is
 * constructed which exactly accepts the strings that are to be recognized. In
 * order to check whether a given string contains one of the strings that are being
 * searched for the constructed automaton has to applied on each position in the
 * given string.
 **/
class KatePrefixStore {
  public:
    typedef QPair<bool, bool> BooleanPair;

    KatePrefixStore();
    virtual ~KatePrefixStore();

    void addPrefix(const QString& prefix);
    void removePrefix(const QString& prefix);

    /**
     * Returns the shortest prefix of the given string that is contained in
     * this prefix store starting at position 'start'.
     **/
    QString findPrefix(const QString& s, int start = 0) const;

    /**
     * Returns the shortest prefix of the given string that is contained in
     * this prefix store starting at position 'start'.
     **/
    QString findPrefix(const Kate::TextLine& line, int start = 0) const;

    int longestPrefixLength() const;

    void clear();

    void dump();

  protected:
    int m_longestPrefixLength;
    QSet<QString> m_prefixSet;

    // State x Char -> Nr. of char occurrences in prefixes x State
    typedef QHash<unsigned short, QPair<unsigned int, unsigned long long> > CharToOccurrenceStateHash;
    typedef QHash<unsigned long long, CharToOccurrenceStateHash> TransitionFunction;
    TransitionFunction m_transitionFunction;
    QSet<unsigned long long> m_acceptingStates;
    QList<unsigned long long> m_stateFreeList;
    unsigned long long m_lastAssignedState;

    int computeLongestPrefixLength();
    unsigned long long nextFreeState();
//     bool containsPrefixOfLengthEndingWith(int length, const QChar& c);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
