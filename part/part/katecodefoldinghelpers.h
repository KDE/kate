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

#ifndef _KATE_CODEFOLDING_HELPERS_
#define _KATE_CODEFOLDING_HELPERS_

//BEGIN INCLUDES + FORWARDS
#include <q3ptrlist.h>
#include <qobject.h>
#include <QHash>
#include <QList>
#include <QVector>

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
};

class KateCodeFoldingNode
{
  friend class KateCodeFoldingTree;

  public:
    KateCodeFoldingNode ();
    KateCodeFoldingNode (KateCodeFoldingNode *par, signed char typ, unsigned int sLRel);

    ~KateCodeFoldingNode ();

    inline int nodeType () { return type;}

    inline bool isVisible () {return visible;}

    inline KateCodeFoldingNode *getParentNode () {return parentNode;}

    bool getBegin (KateCodeFoldingTree *tree, KTextEditor::Cursor* begin);
    bool getEnd (KateCodeFoldingTree *tree, KTextEditor::Cursor *end);

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

  /**
   * data members
   */
  private:
    KateCodeFoldingNode                *parentNode;
    unsigned int startLineRel;
    unsigned int endLineRel;

    unsigned int startCol;
    unsigned int endCol;

    bool startLineValid;
    bool endLineValid;

    signed char type;                // 0 -> toplevel / invalid
    bool visible;
    bool deleteOpening;
    bool deleteEnding;

    QVector<KateCodeFoldingNode*> m_children;
};

class KateCodeFoldingTree : public QObject
{
  friend class KateCodeFoldingNode;

  Q_OBJECT

  public:
    KateCodeFoldingTree (KateBuffer *buffer);
    ~KateCodeFoldingTree ();

    KateCodeFoldingNode *findNodeForLine (unsigned int line);

    unsigned int getRealLine         (unsigned int virtualLine);
    unsigned int getVirtualLine      (unsigned int realLine);
    unsigned int getHiddenLinesCount (unsigned int docLine);

    bool isTopLevel (unsigned int line);

    void lineHasBeenInserted (unsigned int line);
    void lineHasBeenRemoved  (unsigned int line);
    void debugDump ();
    void getLineInfo (KateLineInfo *info,unsigned int line);

    unsigned int getStartLine (KateCodeFoldingNode *node);

    void fixRoot (int endLRel);
    void clear ();

    KateCodeFoldingNode *findNodeForPosition(unsigned int line, unsigned int column);
  private:

    KateCodeFoldingNode m_root;

    KateBuffer *m_buffer;

    QHash<int,unsigned int> lineMapping;
    QHash<int,bool>         dontIgnoreUnchangedLines;

    QList<KateCodeFoldingNode*> markedForDeleting;
    QList<KateCodeFoldingNode*> nodesForLine;
    QList<KateHiddenLineBlock>   hiddenLines;

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

  public slots:
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

  signals:
    void regionVisibilityChangedAt  (unsigned int);
    void regionBeginEndAddedRemoved (unsigned int);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
