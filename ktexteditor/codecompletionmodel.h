/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODEL_H
#define KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODEL_H

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QModelIndex>
#include <ktexteditor/range.h>

namespace KTextEditor {

class Document;
class View;

/**
 * \short An item model for providing code completion, and meta information for
 *        enhanced presentation.
 *
 * \todo write documentation
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KTEXTEDITOR_EXPORT CodeCompletionModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    CodeCompletionModel(QObject* parent);
    virtual ~CodeCompletionModel();

    enum Columns {
      Prefix = 0,
      /// Icon representing the type of completion. We have a separate icon field
      /// so that names remain aligned where only some completions have icons,
      /// and so that they can be rearranged by the user.
      Icon,
      Scope,
      Name,
      Arguments,
      Postfix
    };
    static const int ColumnCount = Postfix + 1;

    enum CompletionProperty {
      NoProperty  = 0x0,
      FirstProperty = 0x1,

      // Access specifiers - no more than 1 per item
      Public      = 0x1,
      Protected   = 0x2,
      Private     = 0x4,

      // Extra access specifiers - any number per item
      Static      = 0x8,
      Const       = 0x10,

      // Type - no more than 1 per item (except for Template)
      Namespace   = 0x20,
      Class       = 0x40,
      Struct      = 0x80,
      Union       = 0x100,
      Function    = 0x200,
      Variable    = 0x400,
      Enum        = 0x800,
      Template    = 0x1000,

      // Special attributes - any number per item
      Virtual     = 0x2000,
      Override    = 0x4000,
      Inline      = 0x8000,
      Friend      = 0x10000,
      Signal      = 0x20000,
      Slot        = 0x40000,

      // Scope - no more than 1 per item
      LocalScope      = 0x80000,
      NamespaceScope  = 0x100000,
      GlobalScope     = 0x200000,

      // Keep this in sync so the code knows when to stop
      LastProperty    = GlobalScope
    };
    Q_DECLARE_FLAGS(CompletionProperties, CompletionProperty)

    enum HighlightMethod {
      NoHighlighting        = 0x0,
      InternalHighlighting  = 0x1,
      CustomHighlighting    = 0x2
    };
    Q_DECLARE_FLAGS(HighlightMethods, HighlightMethod)

    /// Meta information is passed through extra {Qt::ItemDataRole}s.
    /// This information should be returned when requested on the Name column.
    enum ExtraItemDataRoles {
      /// The model should return a set of CompletionProperties.
      CompletionRole = Qt::UserRole,

      /// The model should return an index to the scope
      /// -1 represents no scope
      /// \todo how to sort scope?
      ScopeIndex,

      /**
       * If requested, your model should try to determine whether the
       * completion in question is a suitable match for the context (ie.
       * is accessible, exported, + returns the data type required). 
       *
       * The returned data should ideally be matched against the argument-hint context
       * set earlier by SetMatchContext.
       *
       * Return an integer value that should be positive if the completion is suitable,
       * and zero if the completion is not suitable. The value should be between 0 an 10, where
       * 10 means perfect match.
       *
       * Return QVariant::Invalid if you are unable to determine this.
       */
      MatchQuality,

      /**
       * Is requested before MatchQuality(..) is requested. The item on which this is requested
       * is an argument-hint item(@see ArgumentHintDepth). When this role is requested, the item should
       * be noted, and whenever MatchQuality is requested, it should be computed by matching the item given
       * with MatchQuality into the context chosen by SetMatchContext.
       *
       * Feel free to ignore this, but ideally you should return QVariant::Invalid to make clear that your model does not support this.
       * */
      SetMatchContext,
      
      /**
       * Define which highlighting method will be used:
       * - QVariant::Invalid - allows the editor to choose (usually internal highlighting)
       * - QVariant::Integer - highlight as specified by HighlightMethods.
       */
      HighlightingMethod,

      /**
       * Allows an item to provide custom highlighting.  Return a
       * QList\<QVariant\> in the following format:
       * - int startColumn (where 0 = start of the completion entry)
       * - int endColumn (note: not length)
       * - QTextFormat attribute (note: attribute may be a KTextEditor::Attribute, as it is a child class)
       *
       * Repeat this triplet as many times as required, however each column must be >= the previous,
       * and startColumn != endColumn.
       */
      CustomHighlight,

      /**
       * Returns the inheritance depth of the completion.  For example, a completion
       * which comes from the base class would have depth 0, one from a parent class
       * would have depth 1, one from that class' parent 2, etc.
       */
      InheritanceDepth,

      /**
       * This allows items in the completion-list to be expandable. If a model returns an QVariant bool value
       * that evaluates to true, the completion-widget will draw a handle to expand the item, and will also make
       * that action accessible through keyboard.
       */
      IsExpandable,
      /**
       * After a model returned true for a row on IsExpandable, the row may be expanded by the user.
       * When this happens, ExpandingWidget is requested.
       *
       * The model may return two types of values:
       * QWidget*:
       *  If the model returns a QVariant of type QWidget*, the code-completion takes over the given widget
       *  and embeds it into the completion-list under the completion-item. The widget will be automatically deleted at some point.
       *  The completion-widget will use the height of the widget as a hint for it's preferred size, but it will
       *  resize the widget at will.
       * QString:
       *  If the mode returns a QVariant of type QString, it will create a small html-widget showing the given html-code,
       *  and embed it into the completion-list under the completion-item.
       *
       * Warning:
       *   QWidget* widget;
       *   return QVariant(widget);
       * Will not work correctly!
       * Use the following instead.:
       *   QVariant v;
       *   v.setValue<QWidget*>(widget);
       *   return v;
       *
       * */
      ExpandingWidget,
      /**
       * Whenever an item is selected, this will be requested from the underlying model.
       * It may be used as a simple notification that the item was selected.
       *
       * Above that, the model may return a QString, which then should then contain html-code. A html-widget
       * will then be displayed as a one- or two-liner under the currently selected item(it will be partially expanded)
       * */
      ItemSelected,

       /**Is this completion-item an argument-hint?
        * The model should return an integral positive number if the item is an argument-hint, and QVariant() or 0 if it is not one.
        *
        * The returned depth-integer is important for sorting and matching.
        * Example:
        * "otherFunction(function1(function2("
        * all functions named function2 should have ArgumentHintDepth 1, all functions found for function1 should have ArgumentHintDepth 2,
        * and all functions named otherFunction should have ArgumentHintDepth 3
        *
        * Later, a completed item may then be matched with the first argument of function2, the return-type of function2 with the first
        * argument-type of function1, and the return-type of function1 with the argument-type of otherFunction.
        *
        * If the model returns a positive value on this role for a row, the content will be treated specially:
        * - It will be shown in a separate argument-hint list
        * - It will be sorted by Argument-hint depth
        * - Match-qualities will be illustrated by differently highlighting the matched argument if possible
        * The argument-hint list strings will be built from the following source-model columns:
        * Prefix - Should be all text of the function-signature up to left of the matched argument of the function
        * Name - Should be the type and name of the function's matched argument. This part will be highlighted differently depending on the match-quality
        * Suffix - All the text of the function-signature behind the matched argument
        *
        * Example: You are matching a function with signature "void test(int param1, int param2)", and you are matching the first argument.
        * The model should then return:
        * Prefix: "void test("
        * Name: "int param1"
        * Suffix: ", int param2)"
        *
        * If you don't use the highlighting, matching, etc. you can as well return the whole signature on either Prefix, Name, or Suffix.
       * */
      ArgumentHintDepth
    };
    static const int LastItemDataRole = InheritanceDepth;

    void setRowCount(int rowCount);

    enum InvocationType {
      AutomaticInvocation,
      UserInvocation,
      ManualInvocation
    };

    virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);
    virtual void executeCompletionItem(Document* document, const Range& word, int row) const;

    // Reimplementations
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

  private:
    class CodeCompletionModelPrivate* const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::CompletionProperties)
Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::HighlightMethods)

}

#endif // KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODEL_H
