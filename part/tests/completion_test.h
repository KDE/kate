/* This file is part of the KDE libraries
   Copyright (C) 2008 Niko Sams <niko.sams\gmail.com>

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
 
#ifndef KATE_COMPLETIONTEST_H
#define KATE_COMPLETIONTEST_H

#include <QtCore/QObject>

namespace KTextEditor {
  class Document;
}
class KateView;

class CompletionTest : public QObject
{
  Q_OBJECT

  public:
    CompletionTest() {}
    virtual ~CompletionTest() {}

  private Q_SLOTS:
    void init();
    void cleanup();
    void testFilterEmptyRange();
    void testFilterWithRange();
    void testCustomRange1();
    void testCustomRange2();
    void testCustomRangeMultipleModels();
    void testAbortCursorMovedOutOfRange();
    void testAbortInvalidText();
    void testAbortController();
    void testAbortControllerMultipleModels();
    void testEmptyFilterString();
    void testUpdateCompletionRange();
    void testCustomStartCompl();
    void testKateCompletionModel();
    void testAbortImmideatelyAfterStart(); 

  private:
    KTextEditor::Document* m_doc;
    KateView *m_view;
};

#endif
