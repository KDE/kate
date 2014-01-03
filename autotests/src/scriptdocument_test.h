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

#ifndef KATE_SCRIPTDOCUMENT_TEST_H
#define KATE_SCRIPTDOCUMENT_TEST_H

#include <QtCore/QObject>

namespace KTextEditor {
  class View;
}

class KateDocument;
class KateScriptDocument;

class ScriptDocumentTest : public QObject
{
  Q_OBJECT

  public:
    ScriptDocumentTest();
    virtual ~ScriptDocumentTest();

  private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testRfind_data();
    void testRfind();

  private:
    KateDocument *m_doc;
    KTextEditor::View *m_view;
    KateScriptDocument *m_scriptDoc;

  public:
    static QtMsgHandler s_msgHandler;
};

#endif
