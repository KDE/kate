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

#include "katecodefoldinghelpers.h"
#include "katecodefoldinghelpers.moc"

#include "katebuffer.h"
#include "katecursor.h"
#include <kdebug.h>

#include <qstring.h>

#define JW_DEBUG 0

bool KateCodeFoldingTree::trueVal = true;

KateCodeFoldingNode::KateCodeFoldingNode() :
    parentNode(0),
    startLineRel(0),
    endLineRel(0),
    startCol(0),
    endCol(0),
    startLineValid(false),
    endLineValid(false),
    type(0),
    visible(true),
    deleteOpening(false),
    deleteEnding(false)
{
}//the endline fields should be initialised to not valid

KateCodeFoldingNode::KateCodeFoldingNode(KateCodeFoldingNode *par, signed char typ, unsigned int sLRel):
    parentNode(par),
    startLineRel(sLRel),
    endLineRel(10000),
    startCol(0),
    endCol(0),
    startLineValid(true),
    endLineValid(false),
    type(typ),
    visible(true),
    deleteOpening(false),
    deleteEnding(false)
{
}//the endline fields should be initialised to not valid

KateCodeFoldingNode::~KateCodeFoldingNode()
{
  // delete all child nodes
  clearChildren ();
}

bool KateCodeFoldingNode::getBegin(KateCodeFoldingTree *tree, KTextEditor::Cursor* begin) {
  if (!startLineValid) return false;
  unsigned int line=startLineRel;
  for (KateCodeFoldingNode *n=parentNode;n;n=n->parentNode)
    line+=n->startLineRel;

  tree->m_buffer->codeFoldingColumnUpdate(line);
  begin->setLine(line);
  begin->setColumn(startCol);

  return true;
}

bool KateCodeFoldingNode::getEnd(KateCodeFoldingTree *tree, KTextEditor::Cursor *end) {
  if (!endLineValid) return false;
  unsigned int line=startLineRel+endLineRel;
  for (KateCodeFoldingNode *n=parentNode;n;n=n->parentNode)
    line+=n->startLineRel;

  tree->m_buffer->codeFoldingColumnUpdate(line);
  end->setLine(line);
  end->setColumn(endCol);

  return true;
}

int KateCodeFoldingNode::cmpPos(KateCodeFoldingTree *tree, uint line,uint col) {
    KTextEditor::Cursor cur(line,col);
    KTextEditor::Cursor start,end;
    kdDebug(13000)<<"KateCodeFoldingNode::cmpPos (1)"<<endl;
    bool startValid=getBegin(tree, &start);
    kdDebug(13000)<<"KateCodeFoldingNode::cmpPos (2)"<<endl;
    bool endValid=getEnd(tree, &end);
    kdDebug(13000)<<"KateCodeFoldingNode::cmpPos (3)"<<endl;
    if ((!endValid) && startValid) {
      return ((start>cur)?-1:0);
    }
    if ((!startValid) && endValid) {
      return ((cur>end)?1:0);
    }
    //here both have to be valid, both invalid must not happen
    Q_ASSERT(startValid && endValid);
    return  ( (cur<start)?(-1):( (cur>end) ? 1:0));
}

void KateCodeFoldingNode::insertChild (uint index, KateCodeFoldingNode *node)
{
  uint s = m_children.size ();

  if (index > s)
    return;

  m_children.resize (++s);

  for (uint i=s-1; i > index; --i)
    m_children[i] = m_children[i-1];

  m_children[index] = node;
}

KateCodeFoldingNode *KateCodeFoldingNode::takeChild (uint index)
{
  uint s = m_children.size ();

  if (index >= s)
    return 0;

  KateCodeFoldingNode *n = m_children[index];

  for (uint i=index; (i+1) < s; ++i)
    m_children[i] = m_children[i+1];

  m_children.resize (s-1);

  return n;
}

void KateCodeFoldingNode::clearChildren ()
{
  for (int i=0; i < m_children.size(); ++i)
    delete m_children[i];

  m_children.resize (0);
}

KateCodeFoldingTree::KateCodeFoldingTree(KateBuffer *buffer): QObject(buffer), m_buffer (buffer)
{
  clear();
}

void KateCodeFoldingTree::fixRoot(int endLRel)
{
  m_root.endLineRel = endLRel;
}

void KateCodeFoldingTree::clear()
{
  m_root.clearChildren();

  // initialize the root "special" node
  m_root.startLineValid=true;
  m_root.endLineValid=true; // temporary, should be false;
  m_root.endLineRel=1;      // temporary;

  hiddenLinesCountCacheValid=false;
  lineMapping.setAutoDelete(true);
  hiddenLines.clear();
  lineMapping.clear();
  nodesForLine.clear();
  markedForDeleting.clear();
  dontIgnoreUnchangedLines.clear();
}

KateCodeFoldingTree::~KateCodeFoldingTree()
{
}

bool KateCodeFoldingTree::isTopLevel(unsigned int line)
{
  if (m_root.noChildren())
    return true; // no childs

  // look if a given lines belongs to a sub node
  for ( uint i=0; i < m_root.childCount(); ++i )
  {
    KateCodeFoldingNode *node = m_root.child(i);

    if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel))
      return false;  // the line is within the range of a subnode -> return toplevel=false
  }

  return true;  // the root node is the only node containing the given line, return toplevel=true
}

void KateCodeFoldingTree::getLineInfo(KateLineInfo *info, unsigned int line)
{
  // Initialze the returned structure, this will also be returned if the root node has no child nodes
  // or the line is not within a childnode's range.
  info->topLevel = true;
  info->startsVisibleBlock = false;
  info->startsInVisibleBlock = false;
  info->endsBlock = false;
  info->invalidBlockEnd = false;

  if (m_root.noChildren())
    return;

  //let's look for some information
  for ( uint i=0; i < m_root.childCount(); ++i )
  {
    KateCodeFoldingNode *node = m_root.child(i);

    if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel)) // we found a node, which contains the given line -> do a complete lookup
    {
      info->topLevel = false; //we are definitly not toplevel
      findAllNodesOpenedOrClosedAt(line); //lookup all nodes, which start or and at the given line

      for ( KateCodeFoldingNode *node = nodesForLine.first(); node; node = nodesForLine.next() )
      {
        uint startLine = getStartLine(node);

        // type<0 means, that a region has been closed, but not opened
        // eg. parantheses missmatch
        if (node->type < 0)
          info->invalidBlockEnd=true;
        else
        {
          if (startLine != line)  // does the region we look at not start at the given line
            info->endsBlock = true; // than it has to be an ending
          else
          {
            // The line starts a new region, now determine, if it's a visible or a hidden region
            if (node->visible)
              info->startsVisibleBlock=true;
            else
              info->startsInVisibleBlock=true;
          }
        }
      }

      return;
    }
  }

  return;
}


KateCodeFoldingNode *KateCodeFoldingTree::findNodeForLine(unsigned int line)
{
  if (m_root.noChildren()) // does we have child list + nodes ?
    return &m_root;

  // lets look, if given line is within a subnode range, and then return the deepest one.
  for ( uint i=0; i < m_root.childCount(); ++i )
  {
    KateCodeFoldingNode *node = m_root.child(i);

    if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel))
    {
      // a region surounds the line, look in the next deeper hierarchy step
      return findNodeForLineDescending(node,line,0);
    }
  }

  return &m_root;
}


KateCodeFoldingNode *KateCodeFoldingTree::findNodeForLineDescending ( KateCodeFoldingNode *node,
    unsigned int line, unsigned int offset, bool oneStepOnly )
{
  if (node->noChildren())
    return node;

  // calculate the offset, between a subnodes real start line and its relative start
  offset += node->startLineRel;

  for ( uint i=0; i < node->childCount(); ++i )
  {
    KateCodeFoldingNode *subNode = node->child(i);

    if ((subNode->startLineRel+offset<=line) && (line<=subNode->endLineRel+subNode->startLineRel+offset)) //warning fix me for invalid ends
    {
      // a subnode contains the line.
      // if oneStepOnly is true, we don't want to search for the deepest node, just return the found one

      if (oneStepOnly)
        return subNode;
      else
        return findNodeForLineDescending (subNode,line,offset); // look into the next deeper hierarchy step
    }
  }

  return node; // the current node has no sub nodes, or the line couldn'te be found within a subregion
}

KateCodeFoldingNode *KateCodeFoldingTree::findNodeForPosition(unsigned int line, unsigned int column)
{
  KateCodeFoldingNode *node=findNodeForLine(line);

  if (node==&m_root) return &m_root;

  kdDebug(13000)<<"initial cmpPos"<<endl;

  KateCodeFoldingNode *tmp;
  int leq=node->cmpPos(this, line,column);
  while (true) {
    switch (leq) {
      case 0: {
                if (node->noChildren())
                  return node;
                else
                {
                  tmp=node;
                  for ( uint i=0; i < node->childCount(); ++i )
                  {
                    KateCodeFoldingNode *subNode = node->child(i);
                    kdDebug(13000)<<"cmdPos(case0):calling"<<endl;
                    leq=subNode->cmpPos(this, line,column);
                    kdDebug(13000)<<"cmdPos(case0):returned"<<endl;
                    if (leq==0) {
                        tmp=subNode;
                        break;
                    } else if (leq==-1) break;
                  }
                  if (tmp!=node) node=tmp; else return node;
                }
                break;
              }
      //this could be optimized a littlebit
      case -1:
      case 1:  {
                  if (!(node->parentNode)) return &m_root;
                  kdDebug(13000)<<"current node type"<<node->type<<endl;
                  node=node->parentNode;
                  kdDebug(13000)<<"cmdPos(case-1/1):calling:"<<node<<endl;
                  leq=node->cmpPos(this, line,column);
                  kdDebug(13000)<<"cmdPos(case-1/1):returned"<<endl;
                  break;
                }
    }

  }
  Q_ASSERT(false);
  return &m_root;
}

void KateCodeFoldingTree::debugDump()
{
  //dump all nodes for debugging
  kdDebug(13000)<<"The parsed region/block tree for code folding"<<endl;
  dumpNode(&m_root, "");
}

void KateCodeFoldingTree::dumpNode(KateCodeFoldingNode *node, const QString &prefix)
{
  //output node properties
  kdDebug(13000)<<prefix<<QString("Type: %1, startLineValid %2, startLineRel %3, endLineValid %4, endLineRel %5, visible %6").
      arg(node->type).arg(node->startLineValid).arg(node->startLineRel).arg(node->endLineValid).
      arg(node->endLineRel).arg(node->visible)<<endl;

  //output child node properties recursive
  if (node->noChildren())
    return;

  QString newprefix(prefix + "   ");
  for ( uint i=0; i < node->childCount(); ++i )
    dumpNode (node->child(i),newprefix);
}

/*
 That's one of the most important functions ;)
*/
void KateCodeFoldingTree::updateLine(unsigned int line,
  QVector<int> *regionChanges, bool *updated,bool changed,bool colsChanged)
{
  if ( (!changed) || colsChanged)
  {
    if (dontIgnoreUnchangedLines.isEmpty())
      return;

    if (dontIgnoreUnchangedLines[line])
      dontIgnoreUnchangedLines.remove(line);
    else
      return;
  }

  something_changed = false;

  findAndMarkAllNodesforRemovalOpenedOrClosedAt(line);

  if (regionChanges->isEmpty())
  {
    //  KateCodeFoldingNode *node=findNodeForLine(line);
    //  if (node->type!=0)
    //  if (getStartLine(node)+node->endLineRel==line) removeEnding(node,line);
  }
  else
  {
    for (unsigned int i=0;i<regionChanges->size() / 4;i++)
    {
        signed char tmp=(*regionChanges)[regionChanges->size()-2-i*2];
        uint tmppos=(*regionChanges)[regionChanges->size()-1-i*2];
        (*regionChanges)[regionChanges->size()-2-i*2]=(*regionChanges)[i*2];
        (*regionChanges)[regionChanges->size()-1-i*2]=(*regionChanges)[i*2+1];
        (*regionChanges)[i*2]=tmp;
        (*regionChanges)[i*2+1]=tmppos;
    }


    signed char data= (*regionChanges)[regionChanges->size()-2];
    uint charPos=(*regionChanges)[regionChanges->size()-1];
    regionChanges->resize (regionChanges->size()-2);

    int insertPos=-1;
    KateCodeFoldingNode *node = findNodeForLine(line);

    if (data<0)
    {
      //  if (insertPos==-1)
      {
        unsigned int tmpLine=line-getStartLine(node);

        for ( uint i=0; i < node->childCount(); ++i )
        {
          if (node->child(i)->startLineRel >= tmpLine)
          {
            insertPos=i;
            break;
          }
        }
      }
    }
    else
    {
      for (; (node->parentNode) && (getStartLine(node->parentNode)==line) && (node->parentNode->type!=0); node=node->parentNode);

      if ((getStartLine(node)==line) && (node->type!=0))
      {
        insertPos=node->parentNode->findChild(node);
        node = node->parentNode;
      }
      else
      {
        for ( uint i=0; i < node->childCount(); ++i )
        {
          if (getStartLine(node->child(i))>=line)
          {
            insertPos=i;
            break;
          }
        }
      }
    }

    do
    {
      if (data<0)
      {
        if (correctEndings(data,node,line,charPos,insertPos))
        {
          insertPos=node->parentNode->findChild(node)+1;
          node=node->parentNode;
        }
        else
        {
          if (insertPos!=-1) insertPos++;
        }
      }
      else
      {
        int startLine=getStartLine(node);
        if ((insertPos==-1) || (insertPos>=(int)node->childCount()))
        {
          KateCodeFoldingNode *newNode = new KateCodeFoldingNode (node,data,line-startLine);
          something_changed = true;
          node->appendChild(newNode);
          addOpening(newNode, data, regionChanges, line,charPos);
          insertPos = node->findChild(newNode)+1;
        }
        else
        {
          if (node->child(insertPos)->startLineRel == line-startLine)
          {
            addOpening(node->child(insertPos), data, regionChanges, line,charPos);
            insertPos++;
          }
          else
          {
//              kdDebug(13000)<<"ADDING NODE "<<endl;
            KateCodeFoldingNode *newNode = new KateCodeFoldingNode (node,data,line-startLine);
            something_changed = true;
            node->insertChild(insertPos, newNode);
            addOpening(newNode, data, regionChanges, line,charPos);
            insertPos++;
          }
        }
      }

      if (regionChanges->isEmpty())
        data = 0;
      else
      {
        data = (*regionChanges)[regionChanges->size()-2];
        charPos=(*regionChanges)[regionChanges->size()-1];
        regionChanges->resize (regionChanges->size()-2);
      }
    } while (data!=0);
  }

  cleanupUnneededNodes(line);
//  if (something_changed) emit regionBeginEndAddedRemoved(line);
  (*updated) = something_changed;
}


bool KateCodeFoldingTree::removeOpening(KateCodeFoldingNode *node,unsigned int line)
{
  signed char type;
  if ((type=node->type) == 0)
  {
    dontDeleteOpening(node);
    dontDeleteEnding(node);
    return false;
  }

  if (!node->visible)
  {
  toggleRegionVisibility(getStartLine(node));
  }

  KateCodeFoldingNode *parent = node->parentNode;
  int mypos = parent->findChild(node);

  if (mypos > -1)
  {
  //move childnodes() up
  for(; node->childCount()>0 ;)
  {
    KateCodeFoldingNode *tmp;
    parent->insertChild(mypos, tmp=node->takeChild(0));
    tmp->parentNode = parent;
    tmp->startLineRel += node->startLineRel;
    mypos++;
  }

  // remove the node
  //mypos = parent->findChild(node);
  bool endLineValid = node->endLineValid;
  int endLineRel = node->endLineRel;
  uint endCol=node->endCol;

  // removes + deletes
  delete parent->takeChild(mypos);

  if ((type>0) && (endLineValid))
    correctEndings(-type, parent, line+endLineRel/*+1*/,endCol, mypos); // why the hell did I add a +1 here ?
  }

  return true;
}

bool KateCodeFoldingTree::removeEnding(KateCodeFoldingNode *node,unsigned int /* line */)
{
  KateCodeFoldingNode *parent = node->parentNode;

  if (!parent)
    return false;

  if (node->type == 0)
    return false;

  if (node->type < 0)
  {
    // removes + deletes
    int i = parent->findChild (node);
    if (i >= 0)
      delete parent->takeChild (i);

    return true;
  }

  int mypos = parent->findChild(node);
  int count = parent->childCount();

  for (int i=mypos+1; i<count; i++)
  {
    if (parent->child(i)->type == -node->type)
    {
      node->endLineValid = true;
      node->endLineRel = parent->child(i)->startLineRel - node->startLineRel;

      delete parent->takeChild(i);

      count = i-mypos-1;
      if (count > 0)
      {
        for (int i=0; i<count; i++)
        {
          KateCodeFoldingNode *tmp = parent->takeChild(mypos+1);
          tmp->startLineRel -= node->startLineRel;
          tmp->parentNode = node; //should help 16.04.2002
          node->appendChild(tmp);
        }
      }
      return false;
    }
  }

  if ( (parent->type == node->type) || /*temporary fix */ (!parent->parentNode))
  {
    for (int i=mypos+1; i<(int)parent->childCount(); i++)
    {
      KateCodeFoldingNode *tmp = parent->takeChild(mypos+1);
      tmp->startLineRel -= node->startLineRel;
      tmp->parentNode = node; // SHOULD HELP 16.04.2002
      node->appendChild(tmp);
    }

    // this should fix the bug of wrongly closed nodes
    if (!parent->parentNode)
      node->endLineValid=false;
    else
      node->endLineValid = parent->endLineValid;

    node->endLineRel = parent->endLineRel-node->startLineRel;

    if (node->endLineValid)
      return removeEnding(parent, getStartLine(parent)+parent->endLineRel);

    return false;
  }

  node->endLineValid = false;
  node->endLineRel = parent->endLineRel - node->startLineRel;

  return false;
}


bool KateCodeFoldingTree::correctEndings(signed char data, KateCodeFoldingNode *node,unsigned int line,unsigned int endCol,int insertPos)
{
//  if (node->type==0) {kdError()<<"correct Ending should never be called with the root node"<<endl; return true;}
  uint startLine = getStartLine(node);
  if (data != -node->type)
  {
#if JW_DEBUG
    kdDebug(13000)<<"data!=-node->type (correctEndings)"<<endl;
#endif
    //invalid close -> add to unopend list
    dontDeleteEnding(node);
    if (data == node->type) {
      node->endCol=endCol;
      return false;
    }
    KateCodeFoldingNode *newNode = new KateCodeFoldingNode (node,data,line-startLine);
    something_changed = true;
    newNode->startLineValid = false;
    newNode->endLineValid = true;
    newNode->endLineRel = 0;
    newNode->endCol=endCol;

    if ((insertPos==-1) || (insertPos==(int)node->childCount()))
      node->appendChild(newNode);
    else
      node->insertChild(insertPos,newNode);

      // find correct position
    return false;
  }
  else
  {
    something_changed = true;
    dontDeleteEnding(node);

    // valid closing region
    if (!node->endLineValid)
    {
      node->endLineValid = true;
      node->endLineRel = line - startLine;
      node->endCol=endCol;
      //moving

      moveSubNodesUp(node);
    }
    else
    {
#if JW_DEBUG
      kdDebug(13000)<<"Closing a node which had already a valid end"<<endl;
#endif
      // block has already an ending
      if (startLine+node->endLineRel == line)
      {
         node->endCol=endCol;
         // we won, just skip
#if JW_DEBUG
        kdDebug(13000)<< "We won, just skipping (correctEndings)"<<endl;
#endif
      }
      else
      {
        int bakEndLine = node->endLineRel+startLine;
        uint bakEndCol = node->endCol;
        node->endLineRel = line-startLine;
        node->endCol=endCol;

#if JW_DEBUG
        kdDebug(13000)<< "reclosed node had childnodes()"<<endl;
        kdDebug(13000)<<"It could be, that childnodes() need to be moved up"<<endl;
#endif
  moveSubNodesUp(node);

        if (node->parentNode)
        {
          correctEndings(data,node->parentNode,bakEndLine, bakEndCol,node->parentNode->findChild(node)+1); // ????
        }
        else
        {
          //add to unopened list (bakEndLine)
        }
      }
      }
    }
    return true;
}

void KateCodeFoldingTree::moveSubNodesUp(KateCodeFoldingNode *node)
{
        int mypos = node->parentNode->findChild(node);
        int removepos=-1;
        int count = node->childCount();
        for (int i=0; i<count; i++)
          if (node->child(i)->startLineRel >= node->endLineRel)
          {
            removepos=i;
            break;
          }
#if JW_DEBUG
        kdDebug(13000)<<QString("remove pos: %1").arg(removepos)<<endl;
#endif
        if (removepos>-1)
        {
#if JW_DEBUG
          kdDebug(13000)<<"Children need to be moved"<<endl;
#endif
          KateCodeFoldingNode *moveNode;
          if (mypos == (int)node->parentNode->childCount()-1)
          {
            while (removepos<(int)node->childCount())
            {
              node->parentNode->appendChild(moveNode=node->takeChild(removepos));
              moveNode->parentNode = node->parentNode;
              moveNode->startLineRel += node->startLineRel;
            }
          }
          else
          {
            int insertPos=mypos;
            while (removepos < (int)node->childCount())
            {
              insertPos++;
              node->parentNode->insertChild(insertPos, moveNode=node->takeChild(removepos));
              moveNode->parentNode = node->parentNode; // That should solve a crash
              moveNode->startLineRel += node->startLineRel;
            }
          }
        }

}



void KateCodeFoldingTree::addOpening(KateCodeFoldingNode *node,signed char nType, QVector<int>* list,unsigned int line,unsigned int charPos)
{
  uint startLine = getStartLine(node);
  if ((startLine==line) && (node->type!=0))
  {
#if JW_DEBUG
    kdDebug(13000)<<"startLine equals line"<<endl;
#endif
    if (nType == node->type)
    {
#if JW_DEBUG
      kdDebug(13000)<<"Node exists"<<endl;
#endif
      node->deleteOpening = false;
      node->startCol=charPos;
      KateCodeFoldingNode *parent = node->parentNode;

      if (!node->endLineValid)
      {
        int current = parent->findChild(node);
        int count = parent->childCount()-(current+1);
        node->endLineRel = parent->endLineRel - node->startLineRel;

// EXPERIMENTAL TEST BEGIN
// move this afte the test for unopened, but closed regions within the parent node, or if there are no siblings, bubble up
        if (parent)
          if (parent->type == node->type)
          {
            if (parent->endLineValid)
            {
              removeEnding(parent, line);
              node->endLineValid = true;
            }
          }

// EXPERIMENTAL TEST BEGIN

        if (current != (int)parent->childCount()-1)
        {
        //search for an unopened but closed region, even if the parent is of the same type
#ifdef __GNUC__
#warning  "FIXME:  why does this seem to work?"
#endif
//          if (node->type != parent->type)
          {
            for (int i=current+1; i<(int)parent->childCount(); i++)
            {
              if (parent->child(i)->type == -node->type)
              {
                count = (i-current-1);
                node->endLineValid = true;
                node->endLineRel = getStartLine(parent->child(i))-line;
                node->endCol = parent->child(i)->endCol;
                delete parent->takeChild(i);
                break;
              }
            }
          }
//          else
//          {
//            parent->endLineValid = false;
//            parent->endLineRel = 20000;
//          }

          if (count>0)
          {
            for (int i=0;i<count;i++)
            {
              KateCodeFoldingNode *tmp;
              node->appendChild(tmp=parent->takeChild(current+1));
              tmp->startLineRel -= node->startLineRel;
              tmp->parentNode = node;
            }
          }
        }

      }

      addOpening_further_iterations(node, nType, list, line, 0, startLine,node->startCol);

    } //else ohoh, much work to do same line, but other region type
  }
  else
  { // create a new region
    KateCodeFoldingNode *newNode = new KateCodeFoldingNode (node,nType,line-startLine);
    something_changed = true;

    int insert_position=-1;
    for (int i=0; i<(int)node->childCount(); i++)
    {
      if (startLine+node->child(i)->startLineRel > line)
      {
         insert_position=i;
         break;
      }
    }

    int current;
    if (insert_position==-1)
    {
      node->appendChild(newNode);
      current = node->childCount()-1;
    }
    else
    {
      node->insertChild(insert_position, newNode);
      current = insert_position;
    }

//    if (node->type==newNode->type)
//    {
//      newNode->endLineValid=true;
//      node->endLineValid=false;
//      newNode->endLineRel=node->endLineRel-newNode->startLineRel;
//      node->endLineRel=20000; //FIXME

      int count = node->childCount() - (current+1);
      newNode->endLineRel -= newNode->startLineRel;
      if (current != (int)node->childCount()-1)
      {
        if (node->type != newNode->type)
        {
          for (int i=current+1; i<(int)node->childCount(); i++)
          {
            if (node->child(i)->type == -newNode->type)
            {
              count = node->childCount() - i - 1;
              newNode->endLineValid = true;
              newNode->endLineRel = line - getStartLine(node->child(i));
              delete node->takeChild(i);
              break;
            }
          }
        }
        else
        {
          node->endLineValid = false;
          node->endLineRel = 10000;
        }
        if (count > 0)
        {
          for (int i=0;i<count;i++)
          {
            KateCodeFoldingNode *tmp;
            newNode->appendChild(tmp=node->takeChild(current+1));
            tmp->parentNode=newNode;
          }
        }
//      }
    }

    addOpening(newNode, nType, list, line,charPos);

    addOpening_further_iterations(node, node->type, list, line, current, startLine,node->startCol);
  }
}


void KateCodeFoldingTree::addOpening_further_iterations(KateCodeFoldingNode *node,signed char /* nType */, QVector<int>*
    list,unsigned int line,int current, unsigned int startLine,unsigned int charPos)
{
  while (!(list->isEmpty()))
  {
    if (list->isEmpty())
      return;
    else
    {
         signed char data = (*list)[list->size()-2];
         uint charPos=(*list)[list->size()-1];
       list->resize (list->size()-2);

      if (data<0)
      {
#if JW_DEBUG
        kdDebug(13000)<<"An ending was found"<<endl;
#endif

        if (correctEndings(data,node,line,charPos,-1))
          return; // -1 ?

#if 0
        if(data == -nType)
        {
          if (node->endLineValid)
          {
            if (node->endLineRel+startLine==line) // We've won again
            {
              //handle next node;
            }
            else
            { // much moving
              node->endLineRel=line-startLine;
              node->endLineValid=true;
            }
            return;  // next higher level should do the rest
          }
          else
          {
            node->endLineRel=line-startLine;
            node->endLineValid=true;
            //much moving
          }
        } //else add to unopened list
#endif
      }
      else
      {
        bool needNew = true;
        if (current < (int)node->childCount())
        {
          if (getStartLine(node->child(current)) == line)
            needNew=false;
        }
        if (needNew)
        {
          something_changed = true;
          KateCodeFoldingNode *newNode = new KateCodeFoldingNode(node, data, line-startLine);
          node->insertChild(current, newNode);  //find the correct position later
        }

               addOpening(node->child(current), data, list, line,charPos);
        current++;
        //lookup node or create subnode
      }
    }
  } // end while
}

unsigned int KateCodeFoldingTree::getStartLine(KateCodeFoldingNode *node)
{
  unsigned int lineStart=0;
  for (KateCodeFoldingNode *iter=node; iter->type != 0; iter=iter->parentNode)
    lineStart += iter->startLineRel;

  return lineStart;
}


void KateCodeFoldingTree::lineHasBeenRemoved(unsigned int line)
{
  lineMapping.clear();
  dontIgnoreUnchangedLines.insert(line, &trueVal);
  dontIgnoreUnchangedLines.insert(line-1, &trueVal);
  dontIgnoreUnchangedLines.insert(line+1, &trueVal);
  hiddenLinesCountCacheValid = false;
#if JW_DEBUG
  kdDebug(13000)<<QString("KateCodeFoldingTree::lineHasBeenRemoved: %1").arg(line)<<endl;
#endif

//line ++;
  findAndMarkAllNodesforRemovalOpenedOrClosedAt(line); //It's an ugly solution
  cleanupUnneededNodes(line);  //It's an ugly solution

  KateCodeFoldingNode *node = findNodeForLine(line);
//?????  if (node->endLineValid)
  {
    int startLine = getStartLine(node);
    if (startLine == (int)line)
      node->startLineRel--;
    else
    {
      if (node->endLineRel == 0)
        node->endLineValid = false;
      node->endLineRel--;
    }

    int count = node->childCount();
    for (int i=0; i<count; i++)
    {
      if (node->child(i)->startLineRel+startLine >= line)
        node->child(i)->startLineRel--;
    }
  }

  if (node->parentNode)
    decrementBy1(node->parentNode, node);

  for (QList<KateHiddenLineBlock>::iterator it=hiddenLines.begin(); it != hiddenLines.end(); ++it)
  {
    if ((*it).start > line)
      (*it).start--;
    else if ((*it).start+(*it).length > line)
      (*it).length--;
  }
}


void KateCodeFoldingTree::decrementBy1(KateCodeFoldingNode *node, KateCodeFoldingNode *after)
{
  if (node->endLineRel == 0)
    node->endLineValid = false;
  node->endLineRel--;

  for (uint i=node->findChild(after)+1; i < node->childCount(); ++i)
    node->child(i)->startLineRel--;

  if (node->parentNode)
    decrementBy1(node->parentNode,node);
}


void KateCodeFoldingTree::lineHasBeenInserted(unsigned int line)
{
  lineMapping.clear();
  dontIgnoreUnchangedLines.insert(line, &trueVal);
  dontIgnoreUnchangedLines.insert(line-1, &trueVal);
  dontIgnoreUnchangedLines.insert(line+1, &trueVal);
  hiddenLinesCountCacheValid = false;
//return;
#if JW_DEBUG
  kdDebug(13000)<<QString("KateCodeFoldingTree::lineHasBeenInserted: %1").arg(line)<<endl;
#endif

//  findAndMarkAllNodesforRemovalOpenedOrClosedAt(line);
//  cleanupUnneededNodes(line);

  KateCodeFoldingNode *node = findNodeForLine(line);
// ????????  if (node->endLineValid)
  {
    int startLine=getStartLine(node);
    if (node->type < 0)
      node->startLineRel++;
    else
      node->endLineRel++;

    for (uint i=0; i < node->childCount(); ++i)
    {
      KateCodeFoldingNode *iter = node->child(i);

      if (iter->startLineRel+startLine >= line)
        iter->startLineRel++;
    }
  }

  if (node->parentNode)
    incrementBy1(node->parentNode, node);

  for (QList<KateHiddenLineBlock>::iterator it=hiddenLines.begin(); it!=hiddenLines.end(); ++it)
  {
    if ((*it).start > line)
      (*it).start++;
    else if ((*it).start+(*it).length > line)
      (*it).length++;
  }
}

void KateCodeFoldingTree::incrementBy1(KateCodeFoldingNode *node, KateCodeFoldingNode *after)
{
  node->endLineRel++;

  for (uint i=node->findChild(after)+1; i < node->childCount(); ++i)
    node->child(i)->startLineRel++;

  if (node->parentNode)
    incrementBy1(node->parentNode,node);
}


void KateCodeFoldingTree::findAndMarkAllNodesforRemovalOpenedOrClosedAt(unsigned int line)
{
#ifdef __GNUC__
#warning "FIXME:  make this multiple region changes per line save";
#endif
//  return;
  markedForDeleting.clear();
  KateCodeFoldingNode *node = findNodeForLine(line);
  if (node->type == 0)
    return;

  addNodeToRemoveList(node, line);

  while (((node->parentNode) && (node->parentNode->type!=0)) && (getStartLine(node->parentNode)==line))
  {
    node = node->parentNode;
    addNodeToRemoveList(node, line);
  }
#if JW_DEBUG
  kdDebug(13000)<<" added line to markedForDeleting list"<<endl;
#endif
}


void KateCodeFoldingTree::addNodeToRemoveList(KateCodeFoldingNode *node,unsigned int line)
{
  bool add=false;
#ifdef __GNUC__
#warning "FIXME:  make this multiple region changes per line save";
#endif
  unsigned int startLine=getStartLine(node);
  if ((startLine==line) && (node->startLineValid))
  {
    add=true;
    node->deleteOpening = true;
  }
  if ((startLine+node->endLineRel==line) || ((node->endLineValid==false) && (node->deleteOpening)))
  {
    int myPos=node->parentNode->findChild(node); // this has to be implemented nicely
    if ((int)node->parentNode->childCount()>myPos+1)
     addNodeToRemoveList(node->parentNode->child(myPos+1),line);
    add=true;
    node->deleteEnding = true;
  }

  if(add)
  markedForDeleting.append(node);

}


void KateCodeFoldingTree::findAllNodesOpenedOrClosedAt(unsigned int line)
{
  nodesForLine.clear();
  KateCodeFoldingNode *node = findNodeForLine(line);
  if (node->type == 0)
    return;

  unsigned int startLine = getStartLine(node);
  if (startLine == line)
    nodesForLine.append(node);
  else if ((startLine+node->endLineRel == line))
    nodesForLine.append(node);

  while (node->parentNode)
  {
    addNodeToFoundList(node->parentNode, line, node->parentNode->findChild(node));
    node = node->parentNode;
  }
#if JW_DEBUG
  kdDebug(13000)<<" added line to nodesForLine list"<<endl;
#endif
}


void KateCodeFoldingTree::addNodeToFoundList(KateCodeFoldingNode *node,unsigned int line,int childpos)
{
  unsigned int startLine = getStartLine(node);

  if ((startLine==line) && (node->type!=0))
    nodesForLine.append(node);
  else if ((startLine+node->endLineRel==line) && (node->type!=0))
    nodesForLine.append(node);

  for (int i=childpos+1; i<(int)node->childCount(); i++)
  {
    KateCodeFoldingNode *child = node->child(i);

    if (startLine+child->startLineRel == line)
    {
      nodesForLine.append(child);
      addNodeToFoundList(child, line, 0);
    }
    else
      break;
  }
}


void KateCodeFoldingTree::cleanupUnneededNodes(unsigned int line)
{
#if JW_DEBUG
  kdDebug(13000)<<"void KateCodeFoldingTree::cleanupUnneededNodes(unsigned int line)"<<endl;
#endif

//  return;
  if (markedForDeleting.isEmpty())
    return;

  for (int i=0; i<(int)markedForDeleting.count(); i++)
  {
    KateCodeFoldingNode *node = markedForDeleting.at(i);
    if (node->deleteOpening)
      kdDebug(13000)<<"DELETE OPENING SET"<<endl;
    if (node->deleteEnding)
      kdDebug(13000)<<"DELETE ENDING SET"<<endl;

    if ((node->deleteOpening) && (node->deleteEnding))
    {
#if JW_DEBUG
      kdDebug(13000)<<"Deleting complete node"<<endl;
#endif
      if (node->endLineValid)    // just delete it, it has been opened and closed on this line
      {
        int f = node->parentNode->findChild (node);

        if (f >= 0)
          delete node->parentNode->takeChild(f);
      }
      else
      {
        removeOpening(node, line);
        // the node has subnodes which need to be moved up and this one has to be deleted
      }
      something_changed = true;
    }
    else
    {
      if ((node->deleteOpening) && (node->startLineValid))
      {
#if JW_DEBUG
        kdDebug(13000)<<"calling removeOpening"<<endl;
#endif
        removeOpening(node, line);
        something_changed = true;
      }
      else
      {
        dontDeleteOpening(node);

        if ((node->deleteEnding) && (node->endLineValid))
        {
          dontDeleteEnding(node);
          removeEnding(node, line);
          something_changed = true;
        }
        else
          dontDeleteEnding(node);
      }
    }
  }
}

void KateCodeFoldingTree::dontDeleteEnding(KateCodeFoldingNode* node)
{
  node->deleteEnding = false;
}


void KateCodeFoldingTree::dontDeleteOpening(KateCodeFoldingNode* node)
{
  node->deleteOpening = false;
}


void KateCodeFoldingTree::toggleRegionVisibility(unsigned int line)
{
  // hl whole file
  m_buffer->line (m_buffer->count()-1);

  lineMapping.clear();
  hiddenLinesCountCacheValid = false;
  kdDebug(13000)<<QString("KateCodeFoldingTree::toggleRegionVisibility() %1").arg(line)<<endl;

  findAllNodesOpenedOrClosedAt(line);
  for (int i=0; i<(int)nodesForLine.count(); i++)
  {
    KateCodeFoldingNode *node=nodesForLine.at(i);
    if ( (!node->startLineValid) || (getStartLine(node) != line) )
    {
      nodesForLine.remove(i);
      i--;
    }
  }

  if (nodesForLine.isEmpty())
    return;

  nodesForLine.at(0)->visible = !nodesForLine.at(0)->visible;

  if (!nodesForLine.at(0)->visible)
    addHiddenLineBlock(nodesForLine.at(0),line);
  else
  {
    for (QList<KateHiddenLineBlock>::iterator it=hiddenLines.begin(); it!=hiddenLines.end();++it)
      if ((*it).start == line+1)
      {
        hiddenLines.remove(it);
        break;
      }

    updateHiddenSubNodes(nodesForLine.at(0));
  }

  emit regionVisibilityChangedAt(line);
}

void KateCodeFoldingTree::updateHiddenSubNodes(KateCodeFoldingNode *node)
{
  for (uint i=0; i < node->childCount(); ++i)
  {
    KateCodeFoldingNode *iter = node->child(i);

    if (!iter->visible)
      addHiddenLineBlock(iter, getStartLine(iter));
    else
      updateHiddenSubNodes(iter);
  }
}

void KateCodeFoldingTree::addHiddenLineBlock(KateCodeFoldingNode *node,unsigned int line)
{
  KateHiddenLineBlock data;
  data.start = line+1;
  data.length = node->endLineRel-(existsOpeningAtLineAfter(line+node->endLineRel,node)?1:0); // without -1;
  bool inserted = false;

  for (QList<KateHiddenLineBlock>::iterator it=hiddenLines.begin(); it!=hiddenLines.end(); ++it)
  {
    if (((*it).start>=data.start) && ((*it).start<=data.start+data.length-1)) // another hidden block starting at the within this block already exits -> adapt new block
    {
      // the existing block can't have lines behind the new one, because a newly hidden
      //  block has to encapsulate already hidden ones
      it=hiddenLines.remove(it);
      --it;
    }
    else
    {
      if ((*it).start > line)
      {
        hiddenLines.insert(it, data);
        inserted = true;

        break;
      }
    }
  }

  if (!inserted)
    hiddenLines.append(data);
}

bool KateCodeFoldingTree::existsOpeningAtLineAfter(unsigned int line, KateCodeFoldingNode *node)
{
  for(KateCodeFoldingNode *tmp = node->parentNode; tmp; tmp=tmp->parentNode)
  {
    KateCodeFoldingNode *tmp2;
    unsigned int startLine=getStartLine(tmp);

    if ( (tmp->findChild(node)+1) >= tmp->childCount()) return false;
    if ((tmp2 = tmp->child(tmp->findChild(node) + 1))
         && ((tmp2->startLineRel + startLine) == line))
      return true;

    if ((startLine + tmp->endLineRel) > line)
      return false;
  }

  return false;
}


//
// get the real line number for a virtual line
//
unsigned int KateCodeFoldingTree::getRealLine(unsigned int virtualLine)
{
  // he, if nothing is hidden, why look at it ;)
  if (hiddenLines.isEmpty())
    return virtualLine;

  // kdDebug(13000)<<QString("VirtualLine %1").arg(virtualLine)<<endl;

  unsigned int *real=lineMapping[virtualLine];
  if (real)
    return (*real);

  unsigned int tmp = virtualLine;
  for (QList<KateHiddenLineBlock>::const_iterator it=hiddenLines.begin();it!=hiddenLines.end();++it)
  {
    if ((*it).start<=virtualLine)
      virtualLine += (*it).length;
    else
      break;
  }

  // kdDebug(13000)<<QString("Real Line %1").arg(virtualLine)<<endl;

  lineMapping.insert(tmp, new unsigned int(virtualLine));
  return virtualLine;
}

//
// get the virtual line number for a real line
//
unsigned int KateCodeFoldingTree::getVirtualLine(unsigned int realLine)
{
  // he, if nothing is hidden, why look at it ;)
  if (hiddenLines.isEmpty())
    return realLine;

  // kdDebug(13000)<<QString("RealLine--> %1").arg(realLine)<<endl;

  for (int i = hiddenLines.size()-1; i >= 0; --i)
  {
    if (hiddenLines[i].start <= realLine)
      realLine -= hiddenLines[i].length;
    // else
      // break;
  }

  // kdDebug(13000)<<QString("-->virtual Line %1").arg(realLine)<<endl;

  return realLine;
}

//
// get the number of hidden lines
//
unsigned int KateCodeFoldingTree::getHiddenLinesCount(unsigned int doclen)
{
  // he, if nothing is hidden, why look at it ;)
  if (hiddenLines.isEmpty())
    return 0;

  if (hiddenLinesCountCacheValid)
    return hiddenLinesCountCache;

  hiddenLinesCountCacheValid = true;
  hiddenLinesCountCache = 0;

  for (QList<KateHiddenLineBlock>::const_iterator it=hiddenLines.begin(); it!=hiddenLines.end(); ++it)
  {
    if ((*it).start+(*it).length<=doclen)
      hiddenLinesCountCache += (*it).length;
    else
    {
      hiddenLinesCountCache += ((*it).length- ((*it).length + (*it).start - doclen));
      break;
    }
  }

  return hiddenLinesCountCache;
}

void KateCodeFoldingTree::collapseToplevelNodes()
{
  // hl whole file
  m_buffer->line (m_buffer->count()-1);

  if (m_root.noChildren ())
    return;

  for ( uint i=0; i < m_root.childCount(); ++i )
  {
    KateCodeFoldingNode *node = m_root.child(i);

    if (node->visible && node->startLineValid && node->endLineValid)
    {
        node->visible=false;
        lineMapping.clear();
        hiddenLinesCountCacheValid = false;
        addHiddenLineBlock(node,node->startLineRel);
        emit regionVisibilityChangedAt(node->startLineRel);
    }
  }
}

void KateCodeFoldingTree::expandToplevelNodes(int numLines)
{
  // hl whole file
  m_buffer->line (m_buffer->count()-1);

  KateLineInfo line;
  for (int i = 0; i < numLines; i++) {
    getLineInfo(&line, i);

    if (line.startsInVisibleBlock)
      toggleRegionVisibility(i);
  }
}

int KateCodeFoldingTree::collapseOne(int realLine)
{
  // hl whole file
  m_buffer->line (m_buffer->count()-1);

  KateLineInfo line;
  int unrelatedBlocks = 0;
  for (int i = realLine; i >= 0; i--) {
    getLineInfo(&line, i);

    if (line.topLevel && !line.endsBlock)
      // optimisation
      break;

    if (line.endsBlock  && ( line.invalidBlockEnd ) && (i != realLine)) {
      unrelatedBlocks++;
    }

    if (line.startsVisibleBlock) {
      unrelatedBlocks--;
      if (unrelatedBlocks == -1) {
        toggleRegionVisibility(i);
        return i;
      }
    }
  }
  return -1;
}

void KateCodeFoldingTree::expandOne(int realLine, int numLines)
{
  // hl whole file
  m_buffer->line (m_buffer->count()-1);

  KateLineInfo line;
  int blockTrack = 0;
  for (int i = realLine; i >= 0; i--) {
    getLineInfo(&line, i);

    if (line.topLevel)
      // done
      break;

    if (line.startsInVisibleBlock && i != realLine) {
      if (blockTrack == 0)
        toggleRegionVisibility(i);

      blockTrack--;
    }

    if (line.endsBlock)
      blockTrack++;

    if (blockTrack < 0)
      // too shallow
      break;
  }

  blockTrack = 0;
  for (int i = realLine; i < numLines; i++) {
    getLineInfo(&line, i);

    if (line.topLevel)
      // done
      break;

    if (line.startsInVisibleBlock) {
      if (blockTrack == 0)
        toggleRegionVisibility(i);

      blockTrack++;
    }

    if (line.endsBlock)
      blockTrack--;

    if (blockTrack < 0)
      // too shallow
      break;
  }
}

void KateCodeFoldingTree::ensureVisible( uint line )
{
  // first have a look, if the line is really hidden
  bool found=false;
  for (QList<KateHiddenLineBlock>::const_iterator it=hiddenLines.begin();it!=hiddenLines.end();++it)
  {
    if ( ((*it).start<=line)  && ((*it).start+(*it).length>line) )
    {
      found=true;
      break;
    }
  }


  if (!found) return;

  kdDebug(13000)<<"line "<<line<<" is really hidden ->show block"<<endl;

  // it looks like we really have to ensure visibility
  KateCodeFoldingNode *n = findNodeForLine( line );
  do {
    if ( ! n->visible )
      toggleRegionVisibility( getStartLine( n ) );
    n = n->parentNode;
  } while( n );

}

// kate: space-indent on; indent-width 2; replace-tabs on;
