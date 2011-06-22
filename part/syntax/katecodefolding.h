/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _KATE_CODEFOLDING_H_
#define _KATE_CODEFOLDING_H_

//BEGIN INCLUDES + FORWARDS
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QList>
#include <QtCore/QVector>

#include "katepartprivate_export.h"
#include "katecodefoldingabstract.h"

class KateCodeFoldingTree;
namespace KTextEditor { class Cursor; }
class KateBuffer;

class QString;
//END

class KateHiddenLineBlock
{
  public:
    unsigned int start;
    unsigned int length;
};

class KateLineInfo
{
  public:
  bool topLevel;
  bool startsVisibleBlock;
  bool startsInVisibleBlock;
  bool endsBlock;
  bool invalidBlockEnd;
  int depth;
};

class KateCodeFoldingNode
{
  friend class KateCodeFoldingTree;

  public:
    KateCodeFoldingNode ();
    KateCodeFoldingNode (KateCodeFoldingNode *par, signed char typ, unsigned int sLRel);

    ~KateCodeFoldingNode ();

    inline int nodeType () { return type;}        // encoding of type is not so clear

    inline bool isVisible () {return visible;}

    inline KateCodeFoldingNode *getParentNode () {return parentNode;}

    bool getBegin (KateCodeFoldingTree *tree, KTextEditor::Cursor* begin);  // ??
    bool getEnd (KateCodeFoldingTree *tree, KTextEditor::Cursor *end);      // ??

  /**
   * accessors for the child nodes
   */
  protected:
    inline bool noChildren () const { return m_children.isEmpty(); }

    inline int childCount () const { return m_children.size(); }

    inline KateCodeFoldingNode *child (uint index) const { return m_children[index]; }

    inline int findChild (KateCodeFoldingNode *node, uint start = 0) const
    {
      for (int i=start; i < m_children.size(); ++i)
        if (m_children[i] == node)
          return i;

      return -1;
    }

    inline void appendChild (KateCodeFoldingNode *node) { m_children.append (node); }

    void insertChild (uint index, KateCodeFoldingNode *node);

    KateCodeFoldingNode *takeChild (uint index);

    void clearChildren ();

    int cmpPos(KateCodeFoldingTree *tree, uint line, uint col);

    void setAllowDestruction(bool allowDestruction) { this->allowDestruction = allowDestruction; }

    void printNode (int level);
    
  /**
   * data members
   */
  private:
    KateCodeFoldingNode *parentNode;  // parent
    unsigned int        startLineRel; // start line (relative to parent its "start line") - line for "{"
    unsigned int        endLineRel;   // end line (relative to "start line") - line for "}"

    unsigned int startCol;            // start column - column for {
    unsigned int endCol;              // end column - column for }

    bool startLineValid;              // if "{" exists (not used by other classes)
    bool endLineValid;                // if "}" exists (not used by other classes)

    signed char type;                 // 0 -> toplevel / invalid ; 5 = {} ; 4 = comment ; -5 = only "}" ; 1 = indent node
    bool visible;                     // folded / not folded
    bool deleteOpening;               // smth from the prev alg. will not be used
    bool deleteEnding;                // smth from the prev alg. will not be used
    bool allowDestruction;            // smth from the prev alg. will not be used
    
    QVector<KateCodeFoldingNode*> m_children;
};

class KATEPART_TESTS_EXPORT KateCodeFoldingTree : public QObject
{
  friend class KateCodeFoldingNode;

  Q_OBJECT

  public:
    KateCodeFoldingTree (KateBuffer *buffer);
    ~KateCodeFoldingTree ();

    KateCodeFoldingNode *findNodeForLine (unsigned int line);
    KateCodeFoldingNode *findNodeStartingAt(unsigned int line);
    KateCodeFoldingNode *rootNode () { return &m_root; }

    unsigned int getRealLine         (unsigned int virtualLine);
    unsigned int getVirtualLine      (unsigned int realLine);
    unsigned int getHiddenLinesCount (unsigned int docLine);  // get the number of hidden lines

    void printTree ();                                        // my debug method

    bool isTopLevel (unsigned int line);                      // if any node (not root node) contains "line"

    // if more lines were inserted/deleted, methods will be called for every line
    void lineHasBeenInserted (unsigned int line);             // order : 1,2,3
    void lineHasBeenRemoved  (unsigned int line);             // order : 3,2,1
    void debugDump ();                                        // Just named (wrapper) in KateDocument::dumpRegionTree()
    void getLineInfo (KateLineInfo *info,unsigned int line);  // Make clear what KateLineInfo contains

    unsigned int getStartLine (KateCodeFoldingNode *node);

    void fixRoot (int endLRel);                               // set end line for root node
    void clear ();                                            // Clear the whole FoldingTree (and aux structures)

    KateCodeFoldingNode *findNodeForPosition(unsigned int line, unsigned int column);
  private:

    KateCodeFoldingNode m_root;

    KateBuffer *const m_buffer;

    QHash<int,unsigned int> lineMapping;
    QSet<int>         dontIgnoreUnchangedLines;

    QList<KateCodeFoldingNode*> markedForDeleting;
    QList<KateCodeFoldingNode*> nodesForLine;
    QList<KateHiddenLineBlock>  hiddenLines;

    unsigned int hiddenLinesCountCache;
    bool         something_changed;
    bool         hiddenLinesCountCacheValid;

    static bool trueVal;

    KateCodeFoldingNode *findNodeForLineDescending (KateCodeFoldingNode *, unsigned int, unsigned int, bool oneStepOnly=false);

    bool correctEndings (signed char data, KateCodeFoldingNode *node, unsigned int line, unsigned int endCol, int insertPos);

    void dumpNode    (KateCodeFoldingNode *node, const QString &prefix);
    void addOpening  (KateCodeFoldingNode *node, signed char nType,QVector<int>* list, unsigned int line,unsigned int charPos);
    void addOpening_further_iterations (KateCodeFoldingNode *node,signed char nType, QVector<int>*
                                        list,unsigned int line,int current,unsigned int startLine,unsigned int charPos);

    void incrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);
    void decrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);

    void cleanupUnneededNodes (unsigned int line);

    /**
     * if returns true, this node has been deleted !!
     */
    bool removeEnding (KateCodeFoldingNode *node,unsigned int line);

    /**
     * if returns true, this node has been deleted !!
     */
    bool removeOpening (KateCodeFoldingNode *node,unsigned int line);

    void findAndMarkAllNodesforRemovalOpenedOrClosedAt (unsigned int line);
    void findAllNodesOpenedOrClosedAt (unsigned int line);

    void addNodeToFoundList  (KateCodeFoldingNode *node,unsigned int line,int childpos);
    void addNodeToRemoveList (KateCodeFoldingNode *node,unsigned int line);
    void addHiddenLineBlock  (KateCodeFoldingNode *node,unsigned int line);

    bool existsOpeningAtLineAfter(unsigned int line, KateCodeFoldingNode *node);

    void dontDeleteEnding  (KateCodeFoldingNode*);
    void dontDeleteOpening (KateCodeFoldingNode*);

    void updateHiddenSubNodes (KateCodeFoldingNode *node);
    void moveSubNodesUp (KateCodeFoldingNode *node);

    // my debug methods
    void getLineInfo_private(KateLineInfo *info, unsigned int line);

//     void removeParentReferencesFromChilds(KateCodeFoldingNode* node);

  public Q_SLOTS:
    void updateLine (unsigned int line,QVector<int>* regionChanges, bool *updated, bool changed,bool colschanged);
    void toggleRegionVisibility (unsigned int);
    void collapseToplevelNodes ();
    void expandToplevelNodes (int numLines);
    int collapseOne (int realLine);
    void expandOne  (int realLine, int numLines);
    /**
      Ensures that all nodes surrounding @p line are open
    */
    void ensureVisible( uint line );

  private:
    bool m_clearCache;
  Q_SIGNALS:
    void regionVisibilityChangedAt  (unsigned int,bool clearCache);
    void regionBeginEndAddedRemoved (unsigned int);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
