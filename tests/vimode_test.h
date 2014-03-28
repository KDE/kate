/* This file is part of the KDE libraries

   Copyright (C) 2011 Kuzmich Svyatoslav
   Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>

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
#include <ktexteditor/codecompletionmodel.h>

class QLineEdit;
class QLabel;
class QCompleter;
class QMainWindow;


class WindowKeepActive : public QObject
{
  Q_OBJECT
public:
  WindowKeepActive(QMainWindow *mainWindow);
public slots:
  bool eventFilter(QObject* object, QEvent* event);
private:
  QMainWindow *m_mainWindow;
};

class FailsIfSlotNotCalled : public QObject
{
  Q_OBJECT
public:
  FailsIfSlotNotCalled();
  ~FailsIfSlotNotCalled();
public slots:
  void slot();
private:
  bool m_slotWasCalled;
};

class FailsIfSlotCalled : public QObject
{
  Q_OBJECT
public:
  FailsIfSlotCalled(const QString& failureMessage);
public slots:
  void slot();
private:
  const QString m_failureMessage;
};

/**
 * Helper class that mimics some of the behaviour of KDevelop's code completion, in particular
 * whether it performs "bracket merging" on completed function calls e.g. if we complete a call
 * to "functionCall(int a)" at the end of the -> here:
 *
 *  object->(
 *
 * we end up with
 *
 *  object->functionCall(
 *
 * and the cursor placed after the closing bracket: the opening bracket is merged with the existing
 * bracket.
 *
 * However, if we do the same with
 *
 *  object->
 *
 * we end up with
 *
 *  object->functionCall()
 *
 * again with the cursor placed after the opening bracket.  This time, the brackets were not merged.
 *
 * This helper class is used to test how Macros and replaying of last changes works with complex
 * code completion.
 */
class FakeCodeCompletionTestModel : public KTextEditor::CodeCompletionModel
{
    Q_OBJECT
public:
    FakeCodeCompletionTestModel(KTextEditor::View* parent);
    /**
     * List of completions, in sorted order.
     * A string ending with "()" is treated as a call to a function with no arguments.
     * A string ending with "(...)" is treated as a call to a function with at least one argument.  The "..." is not
     * inserted into the text.
     * A string ending with "();" or "(...);" is the same as above, and the semi-colon is added.  Bracket merging
     * never happens with strings ending with ";".
     */
    void setCompletions(const QStringList& completions);
    void setRemoveTailOnComplete(bool removeTailOnCompletion);
    void setFailTestOnInvocation(bool failTestOnInvocation);
    bool wasInvoked();
    void clearWasInvoked();
    /**
     * A more reliable form of setAutomaticInvocationEnabled().
     */
    void forceInvocationIfDocTextIs(const QString& desiredDocText);
    void doNotForceInvocation();;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual void executeCompletionItem(KTextEditor::Document* document, const KTextEditor::Range& word, int row) const;
    KTextEditor::CodeCompletionInterface * cc( ) const;
private:
  void failTest() const;
  QStringList m_completions;
  KateView *m_kateView;
  KTextEditor::Document *m_kateDoc;
  bool m_removeTailOnCompletion;
  bool m_failTestOnInvocation;
  mutable bool m_wasInvoked;
  QString m_forceInvocationIfDocTextIs;
private slots:
  void textInserted(KTextEditor::Document* document, KTextEditor::Range range);
  void textRemoved(KTextEditor::Document* document, KTextEditor::Range range);
  void checkIfShouldForceInvocation();

};

class ViModeTest : public QObject
{
  Q_OBJECT

public:
  ViModeTest();
  ~ViModeTest();

  static void waitForCompletionWidgetToActivate(KateView *kate_view);

private Q_SLOTS:
  void init();

  void NormalModeMotionsTest();
  void NormalModeCommandsTest();
  void NormalModeControlTests();
  void NormalModeNotYetImplementedFeaturesTest();
  void FakeCodeCompletionTests();

  void InsertModeTests();
  void VisualModeTests();
  void ReplaceModeTests();
  void CommandModeTests();
  void VimStyleCommandBarTests();

  void MappingTests();
  void yankHighlightingTests();
  void CompletionTests();
  void visualLineUpDownTests();

  void ScrollViewTests();

  void MacroTests();

  void debuggingTests();
  void keyParsingTests();
  void AltGr();

  void clipboardTests_data();
  void clipboardTests();
private:
  enum Expectation { ShouldPass, ShouldFail };
  void BeginTest(const QString& original_text);
  void FinishTest_(int line, const QString& expected_text, Expectation expectation = ShouldPass, const QString& failureReason = QString());
  /**
   * Send the coded keypresses to the correct widget, attempting to follow Qt's event dispatching rules.
   */
  void TestPressKey(QString str);
  void DoTest_(int line, QString original_text,
                                  QString command,
                                  QString expected_text,
                                  Expectation expectation = ShouldPass,
                                  const QString& failureReason = QString()
             );

  KateDocument *kate_document;
  KateView *kate_view;
  QMainWindow *mainWindow;
  QVBoxLayout *mainWindowLayout;
  KateViInputModeManager *vi_input_mode_manager;

  bool m_firstBatchOfKeypressesForTest;

  QList<Kate::TextRange*> rangesOnFirstLine();
  void ensureKateViewVisible();
  void waitForCompletionWidgetToActivate();

  QLineEdit *emulatedCommandBarTextEdit();
  QLabel *emulatedCommandTypeIndicator();
  KateViEmulatedCommandBar *emulatedCommandBar();
  QLabel *commandResponseMessageDisplay();
  void verifyShowsNumberOfReplacementsAcrossNumberOfLines(int numReplacements, int acrossNumLines);
  void waitForEmulatedCommandBarToHide(long timeout);
  void verifyCursorAt(const KTextEditor::Cursor& expectedCursorPos);

  void verifyTextEditBackgroundColour(const QColor& expectedBackgroundColour);

  QMap<QString, Qt::KeyboardModifier> m_codesToModifiers;
  Qt::KeyboardModifier parseCodedModifier(const QString& string, int startPos, int* destEndOfCodedModifier);

  QMap<QString, Qt::Key> m_codesToSpecialKeys;
    Qt::Key parseCodedSpecialKey(const QString& string, int startPos, int* destEndOfCodedKey);

  void clearAllMappings();

  void clearSearchHistory();
  QStringList searchHistory();

  void clearCommandHistory();
  QStringList commandHistory();

  void clearReplaceHistory();
  QStringList replaceHistory();

  void clearAllMacros();

  QCompleter *emulatedCommandBarCompleter();

  void verifyCommandBarCompletionVisible();
  void verifyCommandBarCompletionsMatches(const QStringList& expectedCompletionList);
  void verifyCommandBarCompletionContains(const QStringList& expectedCompletionList);

  void clearTrackedDocumentChanges();
private slots:
  void textInserted(KTextEditor::Document* document, KTextEditor::Range range);
  void textRemoved(KTextEditor::Document* document, KTextEditor::Range range);
private:
  class DocChange
  {
  public:
    enum ChangeType { TextRemoved, TextInserted };
    DocChange(ChangeType changeType, KTextEditor::Range changeRange, QString newText = QString())
        : m_changeType(changeType),
          m_changeRange(changeRange),
          m_newText(newText)
    {
    }
    ChangeType changeType() const
    {
      return m_changeType;
    }
    KTextEditor::Range changeRange() const
    {
      return m_changeRange;
    }
    QString newText() const
    {
      return m_newText;
    }
  private:
    ChangeType m_changeType;
    KTextEditor::Range m_changeRange;
    QString m_newText;
  };
  QList<DocChange> m_docChanges;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
