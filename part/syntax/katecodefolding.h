/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2011 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2011 Adrian Lungu <adrian.lungu89@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATECODEFOLDINGABSTRACT_H
#define KATECODEFOLDINGABSTRACT_H

//BEGIN INCLUDES + FORWARDS
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QMapIterator>
#include <QtCore/QListIterator>

#include "katepartprivate_export.h"
//#include <ktexteditor/sessionconfiginterface.h>

class KateCodeFoldingTree;
namespace KTextEditor { class Cursor; }
class KateBuffer;
class KConfigGroup;
class KateDocument;
class KateFoldingTest;

class QString;
//END

class KateLineInfo
{
  public:

// if depth == 0
  bool topLevel;

// true if line contains an unfolded start node
  bool startsVisibleBlock;

// true if line contains a folded start node
  bool startsInVisibleBlock;

// true if line contains a valid end node
  bool endsBlock;

// true if line contains an invalid end node
  bool invalidBlockEnd;

// maximum level of a line's node
  int depth;
};

// Class is used to define and compare node's positions
class KateDocumentPosition
{
  public:
    int line;
    int column;

    KateDocumentPosition (int l, int c) :
        line(l),
        column(c)
    {
    }
    KateDocumentPosition (const KateDocumentPosition &pos) :
        line(pos.line),
        column(pos.column)
    {
    }
    KateDocumentPosition () :
        line(0),
        column(0)
    {
    }

    // Operators
    inline bool operator < (const KateDocumentPosition &pos) const
    {
      return (line < pos.line) ? true :
          (line == pos.line) ?
          ((column < pos.column) ? true : false)
            : false;
    }
    inline bool operator > (const KateDocumentPosition &pos) const
    {
      return (line > pos.line) ? true :
          (line == pos.line) ?
          ((column > pos.column) ? true : false)
            : false;
    }
    inline bool operator == (const KateDocumentPosition &pos) const
    {
      return (line == pos.line && column == pos.column) ? true : false;
    }
    inline bool operator != (const KateDocumentPosition &pos) const
    {
      return (line != pos.line || column != pos.column) ? true : false;
    }
    inline bool operator >= (const KateDocumentPosition &pos) const
    {
      return ((*this) > pos || (*this) == pos);
    }
};

class KateCodeFoldingNode: public QObject
{
  friend class KateCodeFoldingTree;

  //data members
  private:
  // node's parent
    KateCodeFoldingNode           *m_parentNode;

  // node's position in document
    KateDocumentPosition          m_position;

  // 0 -> toplevel / invalid ; 5 = {} ; 4 = comment ; -5 = only "}" ; 1/-1 start/end node py style
  // if type > 0 : start node ; if type < 0 : end node
    int                           m_type;

  // node is folded / unfolded
    bool                          m_visible;

  // a property used to calibrate the folding tree
    int                           m_shortage;

  // Node's start children
    QVector<KateCodeFoldingNode*> m_startChildren;

  // Node's end children
    QVector<KateCodeFoldingNode*> m_endChildren;

    int                           m_virtualColumn;

// public methods - Node's interface :
  public:
    KateCodeFoldingNode ();
    KateCodeFoldingNode (KateCodeFoldingNode *par, int typ, KateDocumentPosition pos);

    ~KateCodeFoldingNode ();

    inline int nodeType ()
      { return m_type; }

    inline bool isVisible ()
      { return m_visible; }

    inline KateCodeFoldingNode *getParentNode ()
      { return m_parentNode; }

    bool getBegin (KateCodeFoldingTree *tree, KTextEditor::Cursor* begin);
    bool getEnd (KateCodeFoldingTree *tree, KTextEditor::Cursor *end);

// protected methods - used by FoldingTree
  protected:

// Setters and getters
    inline void setColumn(int newColumn)
      { m_position.column = newColumn; }
    inline void setLine(int newLine)
      { m_position.line = newLine; }

    inline void setColumn(KateCodeFoldingNode *node)
      { setColumn(node->getColumn()); }
    inline void setLine(KateCodeFoldingNode *node)
      { setLine(node->getLine()); }

    inline int getColumn() const
      { return m_position.column; }
    inline int getLine() const
      { return m_position.line; }
    inline KateDocumentPosition getPosition() const
      { return m_position; }
    int getDepth();
    KateCodeFoldingNode* getStartMatching(KateCodeFoldingNode* endNode);
// End of setters and getters

// Children modifiers
    inline bool noStartChildren () const
      { return m_startChildren.isEmpty(); }
    inline bool noEndChildren () const
      { return m_endChildren.isEmpty(); }
    inline bool noChildren () const
      { return noStartChildren() && noEndChildren(); }

    inline int startChildrenCount () const
      { return m_startChildren.size(); }
    inline int endChildrenCount () const
      { return m_endChildren.size(); }

    inline KateCodeFoldingNode* startChildAt (uint index) const
      { return m_startChildren[index]; }
    inline KateCodeFoldingNode* endChildAt (uint index) const
      { return m_endChildren[index]; }

    KateCodeFoldingNode* removeStartChild (int index);
    KateCodeFoldingNode* removeEndChild (int index);

    inline void clearStartChildren ()
      { m_startChildren.clear(); }
    inline void clearEndChildren ()
      { m_endChildren.clear(); }
    inline void clearAllChildren ()
    {
      clearStartChildren();
      clearEndChildren();
    }

    inline bool hasMatch() const
      { return noEndChildren() ? false : true ; }

    inline KateCodeFoldingNode* matchingNode() const
      { return hasMatch() ? endChildAt(0) : 0 ; }

// Methods used to calibrate the Folding Tree :
    void add(KateCodeFoldingNode *node, QVector<KateCodeFoldingNode*> &m_children);
    void addChild(KateCodeFoldingNode *node);
    bool contains(KateCodeFoldingNode *node);
    bool hasBrothers();
    bool isDuplicated(KateCodeFoldingNode *node);
    void mergeChildren(QVector <KateCodeFoldingNode*> &list1, QVector <KateCodeFoldingNode*> &list2);
    bool removeEndDescending(KateCodeFoldingNode *deletedNode);
    bool removeEndAscending(KateCodeFoldingNode *deletedNode);
    bool removeStart(KateCodeFoldingNode *deletedNode);
    KateCodeFoldingNode* removeChild(KateCodeFoldingNode *deletedNode);
    QVector<KateCodeFoldingNode *> removeChildrenInheritedFrom(KateCodeFoldingNode *node);
    void setParent();
    void updateParent(QVector <KateCodeFoldingNode*> newExcess, int newShortage);
    void updateSelf();

// Debug Methods
    void printNode (int level);
};
// End of KateCodeFoldingNode Class

//-------------------------------------------------------//

// Start of KateCodeFoldingTree
class KATEPART_TESTS_EXPORT KateCodeFoldingTree : public QObject
{
  friend class KateCodeFoldingNode;
  friend class KateFoldingTest;

  Q_OBJECT

  private:

  // Tree's root node
    KateCodeFoldingNode*                          m_root;

  // Root's match (a virtual node at the end of the document)
    KateCodeFoldingNode*                          m_rootMatch;

    KateBuffer* const                             m_buffer;

  // A map (line, <Vector of Nodes> from that line) used for easier access to the folding nodes
    QMap < int, QVector <KateCodeFoldingNode*> >  m_lineMapping;

  // A "special" position (invalid position)
    KateDocumentPosition                          INFposition;

  // A list of the hidden nodes (only those nodes that are not found in an already folded area) - sorted (key = nodeLine)
    QList <KateCodeFoldingNode*>                  m_hiddenNodes;

  // Used to save state (close / open ; reload)
    QList <int>                                   m_hiddenLines;
    QList <int>                                   m_hiddenColumns;

  public:
    KateCodeFoldingTree (KateBuffer *buffer);
    ~KateCodeFoldingTree ();

    void incrementBy1 (QVector <KateCodeFoldingNode*> &nodesLine);
    void decrementBy1 (QVector <KateCodeFoldingNode*> &nodesLine);
    void addDeltaToLine (QVector <KateCodeFoldingNode*> &nodesLine, int delta);

    void lineHasBeenInserted (int line, int column);
    void linesHaveBeenRemoved  (int from, int to);

  // Makes clear what KateLineInfo contains
    void getLineInfo (KateLineInfo *info, int line);

    inline KateCodeFoldingNode *rootNode () { return m_root; }

  // find the node that contains this line, root else
    KateCodeFoldingNode *findNodeForLine (int line);

  // find the first start node from the line, 0 else
    KateCodeFoldingNode *findNodeStartingAt(int line);

    int getRealLine         (int virtualLine);
    int getVirtualLine      (int realLine);

  // get the number of hidden lines
    int getHiddenLinesCount (int docLine);

    KateCodeFoldingNode *findNodeForPosition(int line, int column);

    void debugDump ();
    inline int getStartLine (KateCodeFoldingNode *node)
    {
      return node->getLine();
    }

  // set end line for root node
    void fixRoot (int endLRel);

  // Clear the whole FoldingTree (and aux structures)
    void clear ();

  // session stuff
    void readSessionConfig( const KConfigGroup& config );
    void writeSessionConfig( KConfigGroup& config );


  // Debug methods and members
    void printMapping();
    QString treeString;
    void searchThisNode (KateCodeFoldingNode* deletedNode);

  // Will build the output using the tree alg ; Call : buildTreeString(root,1);
    void buildTreeString(KateCodeFoldingNode *node, int level);
  // end of debug...

  protected:
  // Insert Node methods
    inline void insertNode(int nodeType, KateDocumentPosition pos, int virtualColumn)
    {
      nodeType > 0 ? insertStartNode(nodeType,pos, virtualColumn) : insertEndNode(nodeType,pos);
    }
    void insertStartNode(int type, KateDocumentPosition pos, int virtualColumn);
    void insertEndNode(int type, KateDocumentPosition pos);
    void insertNodeIntoMap(KateCodeFoldingNode* newNode);

  // Delete Node methods
    inline void deleteNode (KateCodeFoldingNode* deletedNode)
    {
      deletedNode->m_type > 0 ? deleteStartNode(deletedNode) : deleteEndNode(deletedNode);
    }
    void deleteEndNode (KateCodeFoldingNode* deletedNode);
    void deleteStartNode (KateCodeFoldingNode* deletedNode);
    void deleteNodeFromMap(KateCodeFoldingNode *node);

  // Update position methods
    void changeColumn(KateCodeFoldingNode *node, int newColumn);
    void setColumns (int line, QVector<int> &newColumns, int virtualNodeIndex, int virtualColumn);
    void updateMapping(int line, QVector<int> &newColumns, int virtualNodeIndex, int virtualColumn);
    int hasVirtualColumns(QVector<int> &newColumns);

  // Line depth methods
    int getLineDepth(int line);
    int getLineDepth(int line, bool &validEndings);

  // Tree algorithm metods
    KateCodeFoldingNode* findParent (KateDocumentPosition startingPos,int childType);
    KateCodeFoldingNode* fineNodeAbove (KateDocumentPosition startingPos);
    void sublist(QVector<KateCodeFoldingNode *> &dest, QVector<KateCodeFoldingNode *> &source,
                                              KateDocumentPosition begin, KateDocumentPosition end);
    KateCodeFoldingNode* findNodeAt(KateDocumentPosition position);

    KateCodeFoldingNode* firstNodeFromLine(QVector<KateCodeFoldingNode *> &lineMap);
    KateCodeFoldingNode* lastNodeFromLine(QVector<KateCodeFoldingNode *> &lineMap);

  // Folding / Unfolding methods
    void replaceFoldedNodeWithList(KateCodeFoldingNode* node, QList<KateCodeFoldingNode*> &newFoldedNodes);
    void foldNode (KateCodeFoldingNode* node);
    void unfoldNode (KateCodeFoldingNode* node);

    void collapseLevel (int level, KateCodeFoldingNode* node, int nodeLevel);
    void expandLevel (int level, KateCodeFoldingNode* node, int nodeLevel);

  public Q_SLOTS:
    void updateLine (int line,QVector<int>* regionChanges, bool* updated, bool changed, bool colschanged);
    void toggleRegionVisibility (int);
    void collapseToplevelNodes ();
    void expandToplevelNodes ();
    int collapseOne (int realLine, int column);
    void expandOne  (int realLine, int column);
    void ensureVisible (int l);

  // Save / Load folding state
    void saveFoldingState();
    void applyFoldingState();

  // Collapse / Expand different levels
    void collapseLevel (int level) { collapseLevel(level, 0, 0); }
    void expandLevel (int level) { expandLevel(level, 0, 0); }

  // Expand All Nodes
    void expandAll ();

  // Collapse all multi-line comments
    void collapseAll_dsComments ();

  Q_SIGNALS:
      void regionVisibilityChanged ();
};

#endif // KATECODEFOLDINGABSTRACT_H

// kate: space-indent on; indent-width 2; replace-tabs on;
