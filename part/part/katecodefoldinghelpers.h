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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_CODEFOLDING_HELPERS_
#define _KATE_CODEFOLDING_HELPERS_

//BEGIN INCLUDES + FORWARDS
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qobject.h>
#include <qintdict.h>

class KateCodeFoldingTree;
class QString;
//END

struct hiddenLineBlock
{
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
  public:  
    KateCodeFoldingNode();
    KateCodeFoldingNode(KateCodeFoldingNode *par, signed char typ, unsigned int sLRel);
    ~KateCodeFoldingNode();
    
    inline QPtrList<KateCodeFoldingNode> *childnodes ()
    {
      if (!m_childnodes)
      {
        m_childnodes = new QPtrList<KateCodeFoldingNode>;
        m_childnodes->setAutoDelete (true);
      }
      
      return m_childnodes;
    }
    
    inline bool hasChildNodes ()
    {
      if (!m_childnodes)
        return false;
    
      return !m_childnodes->isEmpty ();
    }

    // temporary public to avoid friend an be able to disallow the access of m_childnodes directly ;)
    KateCodeFoldingNode                *parentNode;
    unsigned int startLineRel;    
    unsigned int endLineRel;

    bool startLineValid;
    bool endLineValid;

    signed char type;                // 0 -> toplevel / invalid
    bool visible;
    bool deleteOpening;
    bool deleteEnding;
    
  protected:
    QPtrList<KateCodeFoldingNode>    *m_childnodes;
};


class KateCodeFoldingTree : public QObject, public KateCodeFoldingNode
{
  Q_OBJECT
  
  public:
    KateCodeFoldingTree (QObject *);
    ~KateCodeFoldingTree ();

    KateCodeFoldingNode *findNodeForLine (unsigned int line);

    unsigned int getRealLine    (unsigned int virtualLine);
    unsigned int getVirtualLine    (unsigned int realLine);
    unsigned int getHiddenLinesCount (unsigned int docLine);

    bool isTopLevel (unsigned int line);

    void lineHasBeenInserted(unsigned int line);
    void lineHasBeenRemoved    (unsigned int line);
    void debugDump();
    void getLineInfo(KateLineInfo *info,unsigned int line);
  
    void clear();
    
  private:
    QIntDict<unsigned int>    lineMapping;
    QIntDict<bool>            dontIgnoreUnchangedLines;

    QPtrList<KateCodeFoldingNode>    markedForDeleting;
    QPtrList<KateCodeFoldingNode>    nodesForLine;
    QValueList<hiddenLineBlock>        hiddenLines;

    unsigned int    hiddenLinesCountCache;
    bool            something_changed;
    bool            hiddenLinesCountCacheValid;

    KateCodeFoldingNode *findNodeForLineDescending(KateCodeFoldingNode *, unsigned int, unsigned int,bool oneStepOnly=false);

    unsigned int getStartLine(KateCodeFoldingNode *node);

    bool correctEndings    (signed char data, KateCodeFoldingNode *node, unsigned int line, int insertPos);

    void dumpNode    (KateCodeFoldingNode *node,QString prefix);
    void addOpening    (KateCodeFoldingNode *node, signed char nType,QMemArray<signed char>* list, unsigned int line);
    void addOpening_further_iterations (KateCodeFoldingNode *node,signed char nType, QMemArray<signed char>*
                                        list,unsigned int line,int current,unsigned int startLine);

    void incrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);
    void decrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);

    void cleanupUnneededNodes (unsigned int line);
    void removeEnding    (KateCodeFoldingNode *node,unsigned int line);
    void removeOpening    (KateCodeFoldingNode *node,unsigned int line);

    void findAndMarkAllNodesforRemovalOpenedOrClosedAt (unsigned int line);
    void findAllNodesOpenedOrClosedAt (unsigned int line);

    void addNodeToFoundList    (KateCodeFoldingNode *node,unsigned int line,int childpos);
    void addNodeToRemoveList(KateCodeFoldingNode *node,unsigned int line);
    void addHiddenLineBlock    (KateCodeFoldingNode *node,unsigned int line);

    void dontDeleteEnding    (KateCodeFoldingNode*);
    void dontDeleteOpening    (KateCodeFoldingNode*);

    void updateHiddenSubNodes (KateCodeFoldingNode *node);


    void moveSubNodesUp(KateCodeFoldingNode *node);
  public slots:
    void updateLine (unsigned int line,QMemArray<signed char>* regionChanges, bool *updated, bool changed);
    void toggleRegionVisibility (unsigned int);
    void collapseToplevelNodes();
    /** 
      Ensures that all nodes surrounding @p line are open
    */
    void ensureVisible( uint line ); 

  signals:
    void setLineVisible (unsigned int, bool);
    void regionVisibilityChangedAt    (unsigned int);
    void regionBeginEndAddedRemoved    (unsigned int);
};

#endif
