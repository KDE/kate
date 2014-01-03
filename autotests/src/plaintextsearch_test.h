/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_PLAINTEXTSEARCH_TEST_H
#define KATE_PLAINTEXTSEARCH_TEST_H

#include <QtCore/QObject>

class KateDocument;
class KatePlainTextSearch;

class PlainTextSearchTest : public QObject
{
  Q_OBJECT

  public:
    PlainTextSearchTest();
    virtual ~PlainTextSearchTest();

  private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testSearchBackward_data();
    void testSearchBackward();

    void testSingleLineDocument_data();
    void testSingleLineDocument();

    void testMultilineSearch_data();
    void testMultilineSearch();

  private:
    KateDocument *m_doc;
    KatePlainTextSearch *m_search;

  public:
    static QtMsgHandler s_msgHandler;
};

#endif
