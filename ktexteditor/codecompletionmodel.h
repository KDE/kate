/* This file is part of the KDE libraries
   Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
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
 * \section compmodel_intro Introduction
 *
 * The CodeCompletionModel is the actual workhorse to provide code completions
 * in a KTextEditor::View. It is meant to be used in conjunction with the
 * CodeCompletionInterface. The CodeCompletionModel is not meant to be used as
 * is. Rather you need to implement a subclass of CodeCompletionModel to actually
 * generate completions appropriate for your type of Document.
 *
 * \section compmodel_implementing Implementing a CodeCompletionModel
 *
 * The CodeCompletionModel is a QAbstractItemModel, and can be subclassed in the
 * same way. It provides default implementations of several members, however, so
 * in most cases (if your completions are essentially a non-hierarchical, flat list
 * of matches) you will only need to overload few virtual functions.
 *
 * \section compmodel_flatlist Implementing a CodeCompletionModel for a flat list
 *
 * For the simple case of a flat list of completions, you will need to:
 *  - Implement completionInvoked() to actually generate/update the list of completion
 * matches
 *  - implement itemData() (or QAbstractItemModel::data()) to return the information that
 * should be displayed for each match.
 *  - use setRowCount() to reflect the number of matches.
 *
 * \section compmodel_roles_columns Columns and roles
 *
 * \todo document the meaning and usage of the columns and roles used by the
 * CodeCompletionInterface
 *
 * \section compmodel_usage Using the new CodeCompletionModel
 *
 * To start using your CodeCompletionModel, refer to CodeCompletionInterface.
 *
 * \section compmodel_controller ControllerInterface to get more control
 *
 * To have more control over code completion implement
 * CodeCompletionModelControllerInterface in your CodeCompletionModel.
 *
 * \see CodeCompletionInterface, CodeCompletionModelControllerInterface
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
      TypeAlias   = 0x2000,

      // Special attributes - any number per item
      Virtual     = 0x4000,
      Override    = 0x8000,
      Inline      = 0x10000,
      Friend      = 0x20000,
      Signal      = 0x40000,
      Slot        = 0x80000,

      // Scope - no more than 1 per item
      LocalScope      = 0x100000,
      NamespaceScope  = 0x200000,
      GlobalScope     = 0x400000,

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
       *   If the attribute is invalid, and the item is an argument-hint, the text will be drawn with
       *   a background-color depending on match-quality, or yellow.
       *   You can use that to mark the actual arguments that are matched in an argument-hint.
       *
       * Repeat this triplet as many times as required, however each column must be >= the previous,
       * and startColumn != endColumn.
       */
      CustomHighlight,

      /**
       * Returns the inheritance depth of the completion.  For example, a completion
       * which comes from the base class would have depth 0, one from a parent class
       * would have depth 1, one from that class' parent 2, etc. you can use this to
       * symbolize the general distance of a completion-item from a user. It will be used
       * for sorting.
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
       *  The completion-widget will use the height of the widget as a hint for its preferred size, but it will
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
        * The argument-hint list strings will be built from all source-model, with a little special behavior:
        * Prefix - Should be all text of the function-signature up to left of the matched argument of the function
        * Name - Should be the type and name of the function's matched argument. This part will be highlighted differently depending on the match-quality
        * Suffix - Should be all the text of the function-signature behind the matched argument
        *
        * Example: You are matching a function with signature "void test(int param1, int param2)", and you are matching the first argument.
        * The model should then return:
        * Prefix: "void test("
        * Name: "int param1"
        * Suffix: ", int param2)"
        *
        * If you don't use the highlighting, matching, etc. you can also return the columns in the usual way.
       * */
      ArgumentHintDepth,
      
      /**
       * This will be requested for each item to ask whether it should be included in computing a best-matches list.
       * If you return a valid positive integer n here, the n best matches will be listed at top of the completion-list separately.
       *
       * This is expensive because all items of the whole completion-list will be tested for their matching-quality, with each of the level 1
       * argument-hints.
       *
       * For that reason the end-user should be able to disable this feature.
       */
      BestMatchesCount,
      
      /**
       * The following three enumeration-values are only used on expanded completion-list items that contain an expanding-widget(@see ExpandingWidget)
       *
       * You can use them to allow the user to interact with the widget by keyboard.
       *
       * AccessibilityNext will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special navigation
       * short-cut to go to navigate to the next position within the expanding-widget(if applicable).
       *
       * Return QVariant(true) if the input was used.
       * */
      AccessibilityNext,
      /**
       * AccessibilityPrevious will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special navigation
       * short-cut to go to navigate to the previous position within the expanding-widget(if applicable).
       *
       * Return QVariant(true) if the input was used.
       * */
      AccessibilityPrevious,
      /**
       * AccessibilityAccept will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special
       * shortcut to trigger the action associated with the position within the expanding-widget the user has navigated to using AccessibilityNext and AccessibilityPrevious.
       *
       * This should return QVariant(true) if an action was triggered, else QVariant(false) or QVariant().
       * */
      AccessibilityAccept,

      /**
       * Using this Role, it is possible to greatly optimize the time needed to process very long completion-lists.
       *
       * In the completion-list, the items are usually ordered by some properties like argument-hint depth,
       * inheritance-depth and attributes. Kate usually has to query the completion-models for these values
       * for each item in the completion-list in order to extract the argument-hints and correctly sort the
       * completion-list. However, with a very long completion-list, only a very small fraction of the items is actually
       * visible.
       *
       * By using a tree structure you can give the items in a grouped order to kate, so it does not need to look at each
       * item and query data in order to initially show the completion-list.
       *
       * This is how it works:
       * - You create a tree-structure for your items
       * - Every inner node of the tree defines one attribute value that all sub-nodes have in common.
       *   - When the inner node is queried for GroupRole, it should return the "ExtraItemDataRoles" that all sub-nodes have in common
       *   - When the inner node is then queried for that exact role, it should return that value.
       *   - No other queries will be done to inner nodes.
       * - Every leaf node stands for an actual item in the completion list.
       *
       * The recommended grouping order is: Argument-hint depth, inheritance depth, attributes.
       *
       * This role can also be used to define completely custom groups, bypassing the editors builtin grouping:
       *  - Return Qt::DisplayRole when GroupRole is requested
       *  - Return the lable text of the group when Qt::DisplayRole
       *   - Optional: Return an integer sorting-value when InheritanceDepth is  requested. This number will
       *               be used to determine the order of the groups. The order of the builtin groups is:
       *               1 = Best Matches, 100 = Local Scope, 200 = Public, 300 = Protected, 400 = Private, 500 = Namespace, 600 = Global
       *               You can pick any arbitrary number to position your group relative to these builtin groups.
       * */
      GroupRole
    };
    static const int LastItemDataRole = AccessibilityAccept;

    void setRowCount(int rowCount);

    enum InvocationType {
      AutomaticInvocation,
      UserInvocation,
      ManualInvocation
    };

    /**
     * This function is responsible to generating / updating the list of current
     * completions. The default implementation does nothing.
     *
     * When implementing this function, remember to call setRowCount() (or implement
     * rowCount()), and to generate the appropriate change notifications (for instance
     * by calling QAbstractItemModel::reset()).
     * @param view The view to generate completions for
     * @param range The range of text to generate completions for
     * @param invocationType How the code completion was triggered
     * */
    virtual void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType);
    /**
     * @deprecated This does not work if your model is hierarchical(@see GroupRole). Use CodeCompletionModel2::executeCompletionItem2 instead.
     *
     * This function is responsible for inserting a selected completion into the
     * document. The default implementation replaces the text that the completions
     * were based on with the Qt::DisplayRole of the Name column of the given match.
     *
     * @param document The document to insert the completion into
     * @param word The Range that the completions are based on (what the user entered
     * so far)
     * @param row The row of the completion match to insert
     * */
    virtual void executeCompletionItem(Document* document, const Range& word, int row) const;

    // Reimplementations
    /**
     * Reimplemented from QAbstractItemModel::columnCount(). The default implementation
     * returns ColumnCount for all indices.
     * */
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    /**
     * Reimplemented from QAbstractItemModel::index(). The default implementation
     * returns a standard QModelIndex as long as the row and column are valid.
     * */
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    /**
     * Reimplemented from QAbstractItemModel::itemData(). The default implementation
     * returns a map with the QAbstractItemModel::data() for all roles that are used
     * by the CodeCompletionInterface. You will need to reimplement either this
     * function or QAbstractItemModel::data() in your CodeCompletionModel.
     * */
    virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    /**
     * Reimplemented from QAbstractItemModel::parent(). The default implementation
     * returns an invalid QModelIndex for all items. This is appropriate for
     * non-hierarchical / flat lists of completions.
     * */
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    /**
     * Reimplemented from QAbstractItemModel::rowCount(). The default implementation
     * returns the value set by setRowCount() for invalid (toplevel) indices, and 0
     * for all other indices. This is appropriate for non-hierarchical / flat lists
     * of completions
     * */
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

    /**
     * This function returns true if the model needs grouping, otherwise false
     * in KDE 4 default value is true, in KDE 5 the default will be false
     * TODO KDE 5
     */
    bool hasGroups() const;

  Q_SIGNALS:
    
    /**
     * Emit this if the code-completion for this model was invoked, some time is needed in order to get the data,
     * and the model is reset once the data is available.
     *
     * This only has an effect if emitted from within completionInvoked(..).
     *
     * This prevents the code-completion list from showing until this model is reset,
     * so there is no annoying flashing in the user-interface resulting from other models
     * supplying their data earlier.
     *
     * @note The implementation may choose to show the completion-list anyway after some timeout
     *
     * @warning If you emit this, you _must_ also reset the model at some point,
     *                  else the code-completion will be completely broken to the user.
     *                  Consider that there may always be additional completion-models apart from yours.
     *
     * @since KDE 4.3
     */
    void waitForReset();
    
    /**
     * Internal
     */
    void hasGroupsChanged(KTextEditor::CodeCompletionModel *model,bool hasGroups);

  protected:
    void setHasGroups(bool hasGroups);

  private:
    class CodeCompletionModelPrivate* const d;
};

/**
 * You must inherit your completion-model from CodeCompletionModel2 if you want to
 * use a hierarchical structure and want to receive execution-feedback.
 * @see CodeCompletionModel::GroupRole
 * */
class KTEXTEDITOR_EXPORT CodeCompletionModel2 : public CodeCompletionModel {
  Q_OBJECT
  public:
    CodeCompletionModel2(QObject* parent);
    /**
      * This function is responsible for inserting a selected completion into the
      * document. The default implementation replaces the text that the completions
      * were based on with the Qt::DisplayRole of the Name column of the given match.
      *
      * @param document the document to insert the completion into
      * @param word the Range that the completions are based on (what the user entered
      * so far)
      * @param index identifies the completion match to insert
      * */
    virtual void executeCompletionItem2(Document* document, const Range& word, const QModelIndex& index) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::CompletionProperties)
Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::HighlightMethods)

}

#endif // KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODEL_H
