/* This file is part of the KDE libraries
   Copyright (C) 2009 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02111-13020, USA.
*/

#ifndef _KATE_REGEXP_H_
#define _KATE_REGEXP_H_

#include <QtCore/QRegExp>

class KateRegExp
{
  public:
    explicit KateRegExp(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive,
        QRegExp::PatternSyntax syntax = QRegExp::RegExp);

    bool isEmpty() const { return m_regExp.isEmpty(); }
    bool isValid() const { return m_regExp.isValid(); }
    QString pattern() const { return m_regExp.pattern(); }
    int numCaptures() const { return m_regExp.numCaptures(); }
    int pos(int nth = 0) const { return m_regExp.pos(nth); }
    QString cap(int nth = 0) const { return m_regExp.cap(nth); }
    int matchedLength() const { return m_regExp.matchedLength(); }

    int indexIn(const QString &str, int offset = 0,
        QRegExp::CaretMode caretMode = QRegExp::CaretAtZero)
    {
      return m_regExp.indexIn(str, offset, caretMode);
    }

    /**
     * This function is a replacement for QRegExp.lastIndexIn that
     * returns the last match that would have been found when
     * searching forwards, which QRegExp.lastIndexIn does not.
     * We need this behavior to allow the user to jump back to
     * the last match.
     *
     * \param str        Text to search in
     * \param offset     Offset (-1 starts from end, -2 from one before the end)
     * \param caretMode  Meaning of caret (^) in the regular expression
     * \return           Index of match or -1 if no match is found
     */
    int lastIndexIn(const QString & str, int offset = -1,
        QRegExp::CaretMode caretMode = QRegExp::CaretAtZero);

    /**
     * Repairs a regular Expression pattern.
     * This is a workaround to make "." and "\s" not match
     * newlines, which currently is the unconfigurable
     * default in QRegExp.
     *
     * \param stillMultiLine  Multi-line after reparation flag
     * \return                Number of replacements done
     */
    int repairPattern(bool & stillMultiLine);

  private:
    QRegExp m_regExp;
};

#endif // KATEREGEXP_H
