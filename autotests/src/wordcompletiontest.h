/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Sven Brauch <svenbrauch@gmail.com>

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

#ifndef KATE_WORDCOMPLETIONTEST_H
#define KATE_WORDCOMPLETIONTEST_H

#include <QObject>
#include <ktexteditor/document.h>

class WordCompletionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void benchWordRetrievalDistinct();
    void benchWordRetrievalSame();
    void benchWordRetrievalMixed();

private:
  KTextEditor::Document* m_doc;
};

#endif // KATE_WORDCOMPLETIONTEST_H
