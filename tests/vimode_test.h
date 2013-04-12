/* This file is part of the KDE libraries

   Copyright (C) 2011 Kuzmich Svyatoslav

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

#ifndef VIMODE_TEST_H
#define VIMODE_TEST_H

#include <QtCore/QObject>

#include <katedocument.h>
#include <kateviinputmodemanager.h>
#include <kateview.h>

class QLineEdit;
class QLabel;
class ViModeTest : public QObject
{
  Q_OBJECT

public:
  ViModeTest();
  ~ViModeTest();

private Q_SLOTS:
  void NormalModeMotionsTest();
  void NormalModeCommandsTest();
  void NormalModeControlTests();
  void NormalModeNotYetImplementedFeaturesTest();

  void InsertModeTests();
  void VisualModeTests();
  void CommandModeTests();
  void VimStyleCommandBarTests();

  void MappingTests();
  void yankHighlightingTests();
  void CompletionTests();

  void debuggingTests();
private:
  enum Expectation { ShouldPass, ShouldFail };
  void BeginTest(const QString& original_text);
  void FinishTest(const QString& expected_text, Expectation expectation = ShouldPass, const QString& failureReason = QString());
  /**
   * Send the coded keypresses to the correct widget, attempting to follow Qt's event dispatching rules.
   */
  void TestPressKey(QString str);
  void DoTest(QString original_text,
                                  QString command,
                                  QString expected_text,
                                  Expectation expectation = ShouldPass,
                                  const QString& failureReason = QString()
             );

  KateDocument *kate_document;
  KateView *kate_view;
  KateViInputModeManager *vi_input_mode_manager;

  QList<Kate::TextRange*> rangesOnFirstLine();
  void ensureKateViewVisible();
  void waitForCompletionWidgetToActivate();

  QLabel *emulatedCommandTypeIndicator();
  QLineEdit *emulatedCommandBarTextEdit();
  void verifyCursorAt(const KTextEditor::Cursor& expectedCursorPos);

  void verifyTextEditBackgroundColour(const QColor& expectedBackgroundColour);

  QMap<QString, Qt::KeyboardModifier> m_codesToModifiers;
  Qt::KeyboardModifier parseCodedModifier(const QString& string, int startPos, int* destEndOfCodedModifier);

  QMap<QString, Qt::Key> m_codesToSpecialKeys;
    Qt::Key parseCodedSpecialKey(const QString& string, int startPos, int* destEndOfCodedKey);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
