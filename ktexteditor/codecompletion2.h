/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef CODECOMPLETION2_H
#define CODECOMPLETION2_H

#include <QAbstractItemModel>

#include <ktexteditor/range.h>

namespace KTextEditor {

class Document;

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

    enum Columns {
      Prefix,
      Scope,
      Name,
      Arguments,
      Postfix
    };
    static const int ColumnCount = Postfix + 1;

    enum CompletionProperty {
      NoProperty  = 0x0,

      // Access specifiers
      Public      = 0x1,
      Protected   = 0x2,
      Private     = 0x4,
      Static      = 0x8,
      Const       = 0x10,

      // Type
      Namespace   = 0x20,
      Class       = 0x40,
      Struct      = 0x80,
      Union       = 0x100,
      Function    = 0x200,
      Variable    = 0x400,
      Enum        = 0x800,
      Template    = 0x1000,

      // Special attributes
      Virtual     = 0x2000,
      Override    = 0x4000,
      Inline      = 0x8000,
      Friend      = 0x10000,
      Signal      = 0x20000,
      Slot        = 0x40000,

      // Scope
      LocalScope      = 0x80000,
      NamespaceScope  = 0x100000,
      GlobalScope     = 0x200000
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
       * is accessible, exported, + returns the data type required)
       *
       * Return a bool, \e true if the completion is suitable, otherwise
       * return \e false.  Return QVariant::Invalid if you are unable to
       * determine this.
       */
      MatchType,

      /**
       * Define which highlighting method will be used:
       * - QVariant::Invalid - allows the editor to choose (usually internal highlighting)
       * - QVariant::Integer - highlight as specified by HighlightMethods.
       */
      HighlightMethod,

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
      CustomHighlight
    };
    static const int LastItemDataRole = CustomHighlight;

    void setRowCount(int rowCount);

    virtual void executeCompletionItem(Document* document, const Range& word, int row);

    // Reimplementations
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

  private:
    int m_rowCount;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::CompletionProperties)
Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::HighlightMethods)

class KTEXTEDITOR_EXPORT CodeCompletionInterface2
{
  public:
    virtual ~CodeCompletionInterface2();

    virtual bool isCompletionActive() const = 0;
    virtual void startCompletion(const Range& word, CodeCompletionModel* model) = 0;
    virtual void abortCompletion() = 0;

    /**
     * Force execution of the currently selected completion.
     */
    virtual void forceCompletion() = 0;

  //signals:
    //void completionExecuted(const Cursor& position, CodeCompletionModel* model, int row);
    //void completionAborted();
};

}

#endif
