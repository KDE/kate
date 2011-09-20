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

#include "katecodefolding.h"
#include "katebuffer.h"
#include "ktexteditor/cursor.h"
#include <ktexteditor/highlightinterface.h>

#include <kconfig.h>
#include <kconfiggroup.h>

// Debug
#include "QtCore/QStack"
#include "QMessageBox"
//

#include "KDebug"

int debugArea() { static int s_area = KDebug::registerArea("Kate (Folding)"); return s_area; }
#define debug() kDebug(debugArea())

KateCodeFoldingNode::KateCodeFoldingNode() :
    m_parentNode(0),
    m_position(0,0),
    m_type(0),
    m_visible(true),
    m_shortage(1),
    m_virtualColumn(0)
{
}

KateCodeFoldingNode::KateCodeFoldingNode(KateCodeFoldingNode *parent, int typ, KateDocumentPosition pos):
    m_parentNode(parent),
    m_position(pos),
    m_type(typ),
    m_visible(true),
    m_shortage(1),
    m_virtualColumn(pos.column)
{
}

KateCodeFoldingNode::~KateCodeFoldingNode()
{
//  m_startChildren.clear();
//  m_endChildren.clear();
}

// All the children must be kept sorted (using their position)
// This method inserts the new child in a children list
void KateCodeFoldingNode::add(KateCodeFoldingNode *node, QVector<KateCodeFoldingNode*> &m_children)
{
  int i;
  for (i = 0 ; i < m_children.size() ; ++i) {
    if (m_children[i]->m_position > node->m_position)
      break;
  }
  m_children.insert(i,node);
}

// This method add the child to its correct list
void KateCodeFoldingNode::addChild(KateCodeFoldingNode *node)
{
  if (node->m_type > 0)
    add(node,m_startChildren);
  else
    add(node,m_endChildren);
}

// Method return true if "node" is a child of the current node
bool KateCodeFoldingNode::contains(KateCodeFoldingNode *node)
{
  if (node->m_type > 0) {
    foreach (KateCodeFoldingNode *child, m_startChildren)
      if (child == node)
        return true;
    return false;
  }
  else if (node->m_type < 0) {
    foreach (KateCodeFoldingNode *child, m_endChildren)
      if (child == node)
        return true;
    return false;
  }
  return false;
}

// Set the "begin" cursor on node's document position
bool KateCodeFoldingNode::getBegin(KateCodeFoldingTree *tree, KTextEditor::Cursor* begin)
{
  Q_UNUSED(tree);

  if (m_type < 1) return false;

  begin->setLine(getLine());
  begin->setColumn(m_virtualColumn);

  return true;
}

// Return the depth of the node (called recursive
int KateCodeFoldingNode::getDepth()
{
  if (m_type == 0)
    return 0;
  return m_parentNode->getDepth() + 1;
}

// Set the "end" cursor on node's matching document position
// Return false if node has no match
bool KateCodeFoldingNode::getEnd(KateCodeFoldingTree *tree, KTextEditor::Cursor *end)
{
  if (m_type == 0) {
    end->setLine(tree->m_rootMatch->getLine());
    end->setColumn(tree->m_rootMatch->getColumn());
  }
  if ( ! hasMatch() ) return false;

  end->setLine(matchingNode()->getLine());
  end->setColumn(matchingNode()->getColumn() + 1);
  // We want to fold "}", so we add (+ 1)

  return true;
}

// Return the node that has "endNode" as a matching node
// return m_root as default
KateCodeFoldingNode* KateCodeFoldingNode::getStartMatching(KateCodeFoldingNode* endNode)
{
  // If node's parent is root node, then we're done
  if (m_type == 0)
    return this;

  // Else the start node that matches with "endNode"
  if (matchingNode() == endNode)
    return this;

  // Else (it was an excess node)
  return m_parentNode->getStartMatching(endNode);
}

// Return true if this node's parent has any start Children
bool KateCodeFoldingNode::hasBrothers() {
  if (m_type == 0)
    return false;
  if (m_parentNode->startChildrenCount() > 0)
    return true;
  return false;
}

// Return true if this node is an excess (shared) end node.
bool KateCodeFoldingNode::isDuplicated(KateCodeFoldingNode *node)
{
  foreach (KateCodeFoldingNode *child, m_startChildren)
    if (child->contains(node))
      return true;
  return false;
}

// Merges two QVectors of children.
// The result is placed in list1. list2 it is not modified.
void KateCodeFoldingNode::mergeChildren(QVector <KateCodeFoldingNode*> &list1, QVector <KateCodeFoldingNode*> &list2)
{
  QVector <KateCodeFoldingNode*> mergedList;
  int index1 = 0, index2 = 0;
  while (index1 < list1.size() && index2 < list2.size()) {
    if (list1[index1]->m_position < list2[index2]->m_position) {
      mergedList.append(list1[index1]);
      ++index1;
    }
    else if (list1[index1]->m_position > list2[index2]->m_position){
      mergedList.append(list2[index2]);
      ++index2;
    }
    // There might be two different nodes with the same possition
    // This must be checked when the merge is done.
    // There might be some duplicates!
    // We assume they are different nodes, so we'll insert them both
    else {
      mergedList.append(list1[index1]);
      mergedList.append(list2[index2]);
      ++index1;
      ++index2;
    }
  }

  for (; index1 < list1.size() ; ++index1)
    mergedList.append(list1[index1]);
  for (; index2 < list2.size() ; ++index2)
    mergedList.append(list2[index2]);

  // We remove the duplicates
  for (index1 = 0 ; index1 < mergedList.size() ; ++ index1) {
    for (index2 = index1 + 1 ; index2 < mergedList.size() ; ++ index2) {
      if (mergedList[index1] == mergedList[index2]) {
        mergedList.remove(index2);
        index2 --;
      }
    }
  }

  list1 = mergedList;
}


// Removes "node" from "children" list (ascending -> to leafs)
// Return true if succeeded
bool KateCodeFoldingNode::removeEndAscending(KateCodeFoldingNode *deletedNode)
{
  int i;

  // We take a look through all the children
  for (i = 0 ; i < m_startChildren.size() ; ++ i) {
    KateCodeFoldingNode* child = m_startChildren[i];

    // If a child contains the deleted node
    if (child->m_endChildren.contains(deletedNode)) {
      child->removeEndAscending(deletedNode);
      int j;
      for (j = 0 ; j < child->m_endChildren.size() ; ++j) {
        if (child->m_endChildren[j] == deletedNode)
          break;
      }
      // We remove the node
      if (j < child->m_endChildren.size())
        child->m_endChildren.remove(j);
    }
  }
  return true;
}

// Removes "node" from "children" list (descening -> to root)
// Return true if succeeded
bool KateCodeFoldingNode::removeEndDescending(KateCodeFoldingNode *deletedNode)
{
  bool found = false;
  int i;

  for (i = 0 ; i < m_endChildren.size() ; ++i) {
    if (m_endChildren[i] == deletedNode) {
      found = true;
      break;
    }
  }
  if (found) {
    m_endChildren.remove(i);

    if (m_endChildren.empty())
      m_shortage = 1;
    if (m_type > 0) {
      m_parentNode->removeEndDescending(deletedNode);
    }
    return true;
  }
  return false;
}

// Removes "node" from "children" list
// Return true if succeeded
bool KateCodeFoldingNode::removeStart(KateCodeFoldingNode *deletedNode)
{
  bool found = false;
  int i;

  for (i = 0 ; i < m_startChildren.size() ; ++i) {
    if (m_startChildren[i] == deletedNode) {
      found = true;
      break;
    }
  }
  if (found) {
    m_startChildren.remove(i);
    return true;
  }
  return false;
}

// Removes "node" from it's parent children list
KateCodeFoldingNode* KateCodeFoldingNode::removeChild(KateCodeFoldingNode *deletedNode)
{
  if (deletedNode->m_type > 0) {
    if (removeStart(deletedNode))
      return deletedNode;
  }
  else {
    if (removeEndDescending(deletedNode))
      return deletedNode;
  }
  return 0;
}

// Removes all children inherited (shared end nodes) from "node"
QVector<KateCodeFoldingNode *> KateCodeFoldingNode::removeChildrenInheritedFrom(KateCodeFoldingNode *node)
{
  int index;
  QVector<KateCodeFoldingNode *> tempList;
  for (index = 0 ; index < m_endChildren.size() ; ++index) {
    if (node->contains(endChildAt(index))) {
      KateCodeFoldingNode* tempNode = removeChild(endChildAt(index));
      if (tempNode)
        tempList.append(tempNode);
      index --;
    }
  }
  return tempList;
}

// This method will ensure that all children have their parent set correct
void KateCodeFoldingNode::setParent()
{
  foreach(KateCodeFoldingNode *child, m_startChildren) {
    child->m_parentNode = this;
  }
  foreach(KateCodeFoldingNode *child, m_endChildren) {
    bool change = true;
    if (isDuplicated(child) == true)
      change = false;

    if (change)
      child->m_parentNode = this;
  }
}

// This method helps recalibrating the tree
void KateCodeFoldingNode::updateParent(QVector <KateCodeFoldingNode *> newExcess, int newShortage)
{
  // Step 1 : parent's endChildren list is updated
  mergeChildren(m_endChildren,newExcess);
  setParent();

  // Step 2 : parent's shortage is updated
  if (newShortage > 0)
    m_shortage = newShortage + 1;
  else if (hasMatch())
    m_shortage = 0;
  else
    m_shortage = 1;

  // Step 3 : if this node is not the root node, the recalibration continues
  if (m_type && this != m_parentNode)
    updateSelf();
}

// This method recalirates the folding tree after a modification (insertion, deletion) occurs.
void KateCodeFoldingNode::updateSelf()
{
  QVector <KateCodeFoldingNode *> excessList;

  // If this node doesn't have shortage, then some children may become its brothers
  if (m_endChildren.size() > 0) {
    for (int i = 0 ; i < startChildrenCount() ; ++i) {
      KateCodeFoldingNode* child = startChildAt(i);

      // If "child" is lower than parent's pair, this node is not it's child anymore.
      if (child->m_position > matchingNode()->m_position) {
        removeStart(child);
        QVector <KateCodeFoldingNode *> temptExcessList;

        // and all the children inhereted from him will be removed
        temptExcessList = removeChildrenInheritedFrom(child);
        mergeChildren(excessList,temptExcessList);

        // go one step behind (because we removed 1 element)
        i --;

        // and the node selected becomes it's broter (same parent)
        m_parentNode->addChild(child);
      }
    }
  }

  // This node has a shortage (all his brothers becomes his children)
  else {
    for (int i = 0 ; i < m_parentNode->startChildrenCount() && hasMatch() == 0 ; ++i) {

      // It's brother is selected
      KateCodeFoldingNode* child = m_parentNode->startChildAt(i);
      // and if this brother is above the current node, then this node is not it's brother anymore, but it's child
      if (child->m_position > m_position) {
        m_parentNode->removeStart(child);

        // go one step behind (because we removed 1 element)
        i --;

        // the node selected becomes it's child
        addChild(child);

        // we take the excess of the new child's endChildren and merge with the current endChildren list
        QVector <KateCodeFoldingNode*> tempList (child->m_endChildren);
        if (tempList.isEmpty() == false) {
          tempList.pop_front();
          mergeChildren(m_endChildren,tempList);
        }
      }
    }
  }

  // if the node has children, then all its children will be inhereted and we rebuild end_children list
  if (startChildrenCount() > 0)
    m_endChildren.clear();
  foreach (KateCodeFoldingNode *child, m_startChildren) {
    // If this is the only child this node has, then ..
    if (child->m_shortage > 0) {
      m_shortage = child->m_shortage + 1;
      break;
    }

    // There might be some more children, so we merge all their endChildren lists
    else if (child->m_endChildren.size() > 0) {
      QVector <KateCodeFoldingNode*> tempList (child->m_endChildren);
      tempList.pop_front();
      mergeChildren(m_endChildren,tempList);
    }
    // should not reach this branch
    else {
        m_shortage = 1;
    }
  }
  setParent();
  if (hasMatch()) {
    m_shortage = 0;
    m_parentNode->removeChild(matchingNode());
  }

  // call updateParent();
  QVector<KateCodeFoldingNode*> tempList (m_endChildren);
  if (!tempList.empty())
      tempList.pop_front();
  mergeChildren(tempList,excessList);
  m_parentNode->updateParent(tempList,m_shortage);
}


// End of KateCodeFoldingNode Class

//-------------------------------------------------------//

// Start of KateCodeFoldingTree


KateCodeFoldingTree::KateCodeFoldingTree(KateBuffer *buffer) :
    m_buffer(buffer),
    INFposition(-10,10)
{
  m_root = new KateCodeFoldingNode(0,0,KateDocumentPosition(0,-1));
  m_rootMatch = new KateCodeFoldingNode(0,0,KateDocumentPosition(0,0));
  m_lineMapping.clear();
  QVector<KateCodeFoldingNode *> tempVector;
  tempVector.append(m_root);
  m_lineMapping.insert(-1,tempVector);
}

KateCodeFoldingTree::~KateCodeFoldingTree()
{
}

void KateCodeFoldingTree::addDeltaToLine(QVector<KateCodeFoldingNode *> &nodesLine, int delta)
{
  foreach (KateCodeFoldingNode *node, nodesLine) {
    node->setLine(node->getLine() + delta);
  }
}

void KateCodeFoldingTree::clear()
{
  // We delete all nodes
  QList<int>keys = m_lineMapping.uniqueKeys();
  keys.pop_front();
  foreach (int key, keys) {
    QVector<KateCodeFoldingNode*> tempLineMap = m_lineMapping[key];
    while (tempLineMap.empty() == false) {
      KateCodeFoldingNode *tempNode = tempLineMap.last();
      tempLineMap.pop_back();
      delete tempNode;
    }
  }
  // Empty the line mapping
  m_lineMapping.clear();
  m_root->clearAllChildren();

  // Empty the hidden lines list
  m_hiddenNodes.clear();

  // We reattach the root to the line mapping
  QVector<KateCodeFoldingNode *> tempVector;
  tempVector.append(m_root);
  m_lineMapping.insert(-1,tempVector);
}

void KateCodeFoldingTree::collapseAll_dsComments()
{
  if (m_root->noStartChildren())
    return;
  
  foreach (KateCodeFoldingNode *child, m_root->m_startChildren) {
    bool isComment = m_buffer->document()->isComment(child->getLine(),child->getColumn());
    if (!m_hiddenNodes.contains(child) && isComment) {
      foldNode(child);
    }
  }
}

void KateCodeFoldingTree::collapseLevel(int level, KateCodeFoldingNode *node, int nodeLevel)
{
  if (!node)
    node = m_root;

  // if "level" was reached, we fold the node
  if (level == nodeLevel) {
    foldNode(node);
    return;
  }

  // if "level" was not reached, then we continue the search
  foreach (KateCodeFoldingNode *child, node->m_startChildren) {
    collapseLevel(level, child, nodeLevel + 1);
  }
}

int KateCodeFoldingTree::collapseOne(int realLine, int column)
{
  KateCodeFoldingNode* nodeToFold = findParent(KateDocumentPosition(realLine,column - 1),1);

  if (nodeToFold == m_root)
    return 0;

  if (m_hiddenNodes.contains(nodeToFold))
    return 0;

  foldNode(nodeToFold);

  return 0;
}

// This method fold all the top level (depth(node) = 1) nodes
void KateCodeFoldingTree::collapseToplevelNodes()
{
  if (m_root->noStartChildren())
    return;

  foreach (KateCodeFoldingNode *child, m_root->m_startChildren) {
    if (!m_hiddenNodes.contains(child)) {
      foldNode(child);
    }
  }
}

// This metods decrements line value by 1
void KateCodeFoldingTree::decrementBy1(QVector<KateCodeFoldingNode *> &nodesLine)
{
  foreach (KateCodeFoldingNode *node, nodesLine) {
    node->setLine(node->getLine() - 1);
  }
}

// This method removes a node from the line map structure
// If the node was the last node from its line, the enitre map entry will be removed
// If the node was folded, the node will be unfolded
void KateCodeFoldingTree::deleteNodeFromMap(KateCodeFoldingNode *node)
{
  QVector<KateCodeFoldingNode *> mapping = m_lineMapping[node->getLine()];
  int index;
  for (index = 0 ; index < mapping.size() ; index ++) {
    if (mapping[index] == node) {
      mapping.remove(index);
      break;
    }
  }
  if (mapping.empty())
    m_lineMapping.remove(node->getLine());
  else
    m_lineMapping[node->getLine()] = mapping;

  if (m_hiddenNodes.contains(node))
    unfoldNode(node);
}

// This method deletes an end node, "deletedNode"
void KateCodeFoldingTree::deleteEndNode(KateCodeFoldingNode* deletedNode)
{
  // step 0 - remove the node from map (unfold if necessary)
  deleteNodeFromMap(deletedNode);

  // step 1 - this node is removed from the tree
  deletedNode->m_parentNode->removeChild(deletedNode);

  // step 2 - recalibrate folding tree starting from parent
  if (deletedNode->m_parentNode->m_type)
    deletedNode->m_parentNode->updateSelf();

  // step 3 - deleted node
  //searchThisNode(deletedNode);
  delete deletedNode;
}

// This method deletes a start node, "deletedNode"
void KateCodeFoldingTree::deleteStartNode(KateCodeFoldingNode* deletedNode)
{
  // step 0 - remove the node from map (unfold if necessary)
  deleteNodeFromMap(deletedNode);

  // step 1 - all its startChildren are inherited by its heir
  KateCodeFoldingNode *heir = fineNodeAbove(deletedNode->m_position);
  heir->mergeChildren(heir->m_startChildren,deletedNode->m_startChildren);

  // step 2 - this node is removed from the tree
  deletedNode->m_parentNode->removeChild(deletedNode);

  // step 3 - parent inherits shortage and endChildren too
  heir->updateParent(deletedNode->m_endChildren,deletedNode->m_shortage - 1);

  // step 4 - node is deleted
  // searchThisNode(deletedNode);
  delete deletedNode;
}

// This methods is called when a line should be visible
// If necessary, a node is unfolded to make it visible.
void KateCodeFoldingTree::ensureVisible(int l)
{
  foreach (KateCodeFoldingNode* node, m_hiddenNodes) {
    KateCodeFoldingNode* matchNode = node->matchingNode();
    if (!matchNode)
      matchNode = m_rootMatch;
    if (node->getLine() < l && l <= matchNode->getLine()) {
      unfoldNode(node);
      break;
    }
  }
}

void KateCodeFoldingTree::expandAll()
{
  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(m_lineMapping);
  QVector <KateCodeFoldingNode*> tempVector;

  // Coppy the lines before "line"
  while (iterator.hasNext()) {
    tempVector = iterator.peekNext().value();
    foreach (KateCodeFoldingNode* node, tempVector) {
      if (node->m_type > 0 && !node->isVisible()) {
        node->m_visible = true;
      }
    }

    iterator.next();
  }

  m_hiddenNodes.clear();

  emit regionVisibilityChanged();
}

void KateCodeFoldingTree::expandLevel(int level, KateCodeFoldingNode *node, int nodeLevel)
{
  if (!node)
    node = m_root;

  // if "level" was reached, we unfold the node (if it's a hidden node)
  if (level == nodeLevel) {
    if (!node->isVisible()) {
      unfoldNode(node);
    }
    return;
  }

  // if "level" was not reached, then we continue the search
  foreach (KateCodeFoldingNode *child, node->m_startChildren) {
    expandLevel(level, child, nodeLevel + 1);
  }
}

void KateCodeFoldingTree::expandOne(int realLine, int column)
{
  KateCodeFoldingNode* nodeToUnfold = findParent(KateDocumentPosition(realLine,column - 1),1);

  if (nodeToUnfold == m_root || nodeToUnfold->isVisible())
    return;

  unfoldNode(nodeToUnfold);
}

// This method unfold the top level (depth(node = 1)) nodes
void KateCodeFoldingTree::expandToplevelNodes()
{
  if (m_root->noStartChildren())
    return;

  foreach (KateCodeFoldingNode *child, m_root->m_startChildren) {
    if (m_hiddenNodes.contains(child)) {
      unfoldNode(child);
    }
  }
}

// Searches for the first start node above
KateCodeFoldingNode* KateCodeFoldingTree::fineNodeAbove(KateDocumentPosition startingPos)
{
  for (int line = startingPos.line ; line >= 0 ; line --) {
    if (!m_lineMapping.contains(line))
      continue;

    QVector <KateCodeFoldingNode*> tempMap = m_lineMapping[line];
    for (int column = tempMap.size() - 1 ; column >= 0 ; column --) {
      // We search for a "start node"
      // We still have to check positions becose the parent might be on the same line
      if (tempMap[column]->m_type > 0 && tempMap[column]->m_position < startingPos)
        return tempMap[column];
    }
  }

  // The root node is the default parent
  return m_root;
}

// Searches for the node placed on a specific position
KateCodeFoldingNode* KateCodeFoldingTree::findNodeAt(KateDocumentPosition position)
{
  QVector <KateCodeFoldingNode*> tempVector = m_lineMapping[position.line];

  foreach (KateCodeFoldingNode *node,tempVector) {
    if (node->m_position == position)
      return node;
  }
  return 0;
}

KateCodeFoldingNode* KateCodeFoldingTree::findNodeForLine(int line)
{
  KateCodeFoldingNode *tempParentNode = m_root;
  bool cont = true;
  while (cont) {
    cont = false;
    if (tempParentNode->noStartChildren())
      return tempParentNode;
    foreach (KateCodeFoldingNode* child, tempParentNode->m_startChildren) {
      // I found a start node on my line
      if (child->getLine() == line)
        return child;

      // The remaining children are below "line"
      if (child->getLine() > line)
        return tempParentNode;

      // We can go to a higher depth
      if (child->getLine() <= line) {

       // if the child has a match
        if (child->hasMatch()) {
          // and the matching node is below "line", then we go to a higher depth
          if (child->matchingNode()->getLine() >= line) {
            tempParentNode = child;
            cont = true;
            break;
          }
        }

        // or if there is no matching node, then we go to a higher depth
        else {
          tempParentNode = child;
          cont = true;
          break;
        }
      }
    }
  }
  return tempParentNode;
}

KateCodeFoldingNode* KateCodeFoldingTree::findNodeForPosition(int l, int c)
{
  return findNodeAt(KateDocumentPosition(l,c));
}

// Search for the parent of the new node that will be created at position startingPos
KateCodeFoldingNode* KateCodeFoldingTree::findParent(KateDocumentPosition startingPos, int childType)
{
  for (int line = startingPos.line ; line >= 0 ; line --) {
    if (!m_lineMapping.contains(line))
      continue;

    QVector <KateCodeFoldingNode*> tempLineMap = m_lineMapping[line];
    for (int i = tempLineMap.size() - 1 ; i >= 0 ; i --) {

      // The parent must be a "start node"
      if (tempLineMap[i]->m_type < 0)
        continue;

      // The parent must be on a lower position that its child
      if (tempLineMap[i]->m_position > startingPos)
        continue;

      // If it's inserted an "end node", then there are no other conditions
      if (childType < 0)
        return tempLineMap[i];

      // If the node has a shortage, we found the parent node
      if (tempLineMap[i]->hasMatch() == false)
        return tempLineMap[i];

      // If this node has a match and its matching node's
      // position is lower then current node, we found the parent
      // The previous version was with m_endChildren.last()
      if (tempLineMap[i]->matchingNode()->m_position > startingPos)
        return tempLineMap[i];
    }
  }

  // The root node is the default parent
  return m_root;
}

// Return the first start node from "line"
KateCodeFoldingNode* KateCodeFoldingTree::findNodeStartingAt(int line)
{
  if (!m_lineMapping.contains(line) || m_lineMapping[line].empty())
    return 0;
  QVector <KateCodeFoldingNode*> tempLineMap = m_lineMapping[line];
  foreach (KateCodeFoldingNode* child, tempLineMap) {
    if (child->m_type > 0)
      return child;
  }
  return 0;
}

KateCodeFoldingNode* KateCodeFoldingTree::firstNodeFromLine(QVector<KateCodeFoldingNode *> &lineMap)
{
  int minColumn = lineMap.first()->getColumn();
  KateCodeFoldingNode* firstNode = lineMap.first();
  foreach (KateCodeFoldingNode* node, lineMap) {
    if (minColumn > node->getColumn()) {
      minColumn = node->getColumn();
      firstNode = node;
    }
  }
  return firstNode;
}

KateCodeFoldingNode* KateCodeFoldingTree::lastNodeFromLine(QVector<KateCodeFoldingNode *> &lineMap)
{
  int maxColumn = lineMap.last()->getColumn();
  KateCodeFoldingNode* lastNode = lineMap.last();
  foreach (KateCodeFoldingNode* node, lineMap) {
    if (maxColumn > node->getColumn()) {
      maxColumn = node->getColumn();
      lastNode = node;
    }
  }
  return lastNode;
}

void KateCodeFoldingTree::debugDump()
{
  printMapping();
  buildTreeString(m_root,1);
  debug()<<treeString;
}

// Sets the root's match line at the end of the document
void KateCodeFoldingTree::fixRoot(int endLine)
{
  m_rootMatch->setLine(endLine);
}

// Gets "line" depth
int KateCodeFoldingTree::getLineDepth(int line)
{
  // If line is invalid, then it is possible to decrease the depth by 1
  int delta = 0;
  while (line >=0 && (!m_lineMapping.contains(line) || m_lineMapping[line].empty())) {
    line --;
    delta = 1;
  }

  if (line < 0)
    return 0;

  // If the line is valid and not empty, getLineDepth() is called
  bool validEndings;
  int retVal = getLineDepth(line, validEndings);

  // If validEndings is true, then the returned value will be decreased by delta
  if (validEndings && retVal > 0)
    return retVal - delta;
  else
    return retVal;
}

int KateCodeFoldingTree::getLineDepth(int line, bool &validEndings)
{
  validEndings = false;

  // If there is a start node on this line,
  // then the depth of this line is the depth of the last start node (biggest depth)
  KateCodeFoldingNode *lastNode = m_lineMapping[line].last();
  if (lastNode->m_type > 0) {
    return lastNode->getDepth();
  }

  // If we don't have a starting node, but we have an end node on this line,
  // then the depth of this line is the depth of the last VALID end node on this line (smallest depth)
  if (m_lineMapping[line].first()->m_type < 0) {
    QVector<KateCodeFoldingNode *> tempLineMap = m_lineMapping[line];
    int index;
    for (index = 0 ; index < tempLineMap.size() ; ++ index) {
      KateCodeFoldingNode* startMatch = tempLineMap[index]->m_parentNode->getStartMatching(tempLineMap[index]);
      if (tempLineMap[index]->m_type > 0 || startMatch->m_type == 0)
        break;
    }
    if (index > 0) {
      validEndings = true;
      KateCodeFoldingNode* startMatch = tempLineMap[index - 1]->m_parentNode->getStartMatching(tempLineMap[index - 1]);
      return startMatch->getDepth();
    }

    // If there are only invalid end nodes, then the depts is "0"
    return 0;
  }
  return 0;
}

// Provides some info about the line (see KateLineInfo for detailes)
void KateCodeFoldingTree::getLineInfo(KateLineInfo *info, int line)
{
  info->depth = 0;
  info->endsBlock = false;
  info->invalidBlockEnd = false;
  info->startsInVisibleBlock = false;
  info->startsVisibleBlock = false;
  info->topLevel = true;

  info->depth = getLineDepth(line);
  if (info->depth) {
    info->topLevel = false;
  }

  // Find out if there are any nodes on this line
  if (m_lineMapping.contains(line) && !m_lineMapping[line].isEmpty()) {
   QVector<KateCodeFoldingNode *> tempLineMap = m_lineMapping[line];
   foreach(KateCodeFoldingNode *node, tempLineMap) {
     // If our node it's a start node
     if (node->m_type > 0) {
       // Check if it is visibile
       if (node->m_visible) {
         info->startsVisibleBlock = true;
       }
       else {
         info->startsInVisibleBlock = true;
       }
     }

     // If our node it's an end node
     else if (node->m_type < 0){
       // check if it's matching node is the rood node (is invalid)
       if (node->m_parentNode->getStartMatching(node)->m_type) {
         info->endsBlock = true;
       }
       else {
         info->invalidBlockEnd = true;
       }
     }
   }
  }
}

// Transforms the "virtualLine" into into a real line
int KateCodeFoldingTree::getRealLine(int virtualLine)
{
  foreach (KateCodeFoldingNode* node, m_hiddenNodes) {
    KateCodeFoldingNode* matchNode = node->matchingNode();
    if (!matchNode)
      matchNode = m_rootMatch;

    // We add (as an offset) the size of all the hidden blocks found above the "virtualLine"
    if (node->getLine() < virtualLine) {
      virtualLine += (matchNode->getLine() - node->getLine());
    }
  }
  return virtualLine;
}

// Transforms the "realLine" into into a virtual line
int KateCodeFoldingTree::getVirtualLine(int realLine)
{
  int offset = 0;
  foreach (KateCodeFoldingNode* node, m_hiddenNodes) {
    KateCodeFoldingNode* matchNode = node->matchingNode();
    if (!matchNode)
      matchNode = m_rootMatch;

    // We subtract the size of the hidden blocks found above the "virtualLine"
    if (matchNode->getLine() <= realLine) {
      offset += (matchNode->getLine() - node->getLine());
    }

    // If the "realLine" is hidden, then the offset substracted will be different
    else if (node->getLine() <= realLine && realLine <= matchNode->getLine())
      offset += (realLine - node->getLine());
  }
  return realLine - offset;
}

// Gets the total size of the hidden blocks
int KateCodeFoldingTree::getHiddenLinesCount(int docLine)
{
  m_rootMatch->setLine(docLine);
  if (m_root->m_visible == false)
    return docLine;
  int n = 0;
  foreach (KateCodeFoldingNode* node, m_hiddenNodes) {
    KateCodeFoldingNode* matchNode = node->matchingNode();
    if (!matchNode) {
      matchNode = m_rootMatch;
      // if the match is end of the doc, then (-1)
      n --;
    }
    n += (matchNode->getLine() - node->getLine());
  }
  return n;
}

// All the node's lines are incremented with 1
void KateCodeFoldingTree::incrementBy1(QVector<KateCodeFoldingNode *> &nodesLine)
{
  foreach (KateCodeFoldingNode *node, nodesLine) {
    node->setLine(node->getLine() + 1);
  }
}

// This method inserts a "newNode" into the lineMapping structure
void KateCodeFoldingTree::insertNodeIntoMap(KateCodeFoldingNode* newNode)
{
  // If this line is not empty
  if (m_lineMapping.contains(newNode->getLine())) {
    QVector<KateCodeFoldingNode *> tempLineMap = m_lineMapping[newNode->getLine()];
    int index;
    for (index = 0 ; index < tempLineMap.size() ; index++) {
      if (tempLineMap[index]->getColumn() > newNode->getColumn())
        break;
    }
    tempLineMap.insert(index,newNode);
    m_lineMapping[newNode->getLine()] = tempLineMap;
  }
  // If line is empty
  else {
    QVector<KateCodeFoldingNode *> tempLineMap;
    tempLineMap.append(newNode);
    m_lineMapping[newNode->getLine()] = tempLineMap;
  }
}

// Inserts a new end node into a specific position, pos
void KateCodeFoldingTree::insertEndNode(int type, KateDocumentPosition pos)
{
  // step 0 - set newNode's parameters
  // find the parent of the new node
  KateCodeFoldingNode *parentNode = findParent(pos,type);
  KateCodeFoldingNode *newNode = new KateCodeFoldingNode(parentNode,type,pos);

  // step1 - add the new node to the map and to its parent node
  insertNodeIntoMap(newNode);
  parentNode->addChild(newNode);

  // step 1 - call updateSelf() for parent node to recalibrate tree
  // (only if node's parent is not the root node)
  if (parentNode->m_type)
    parentNode->updateSelf();
}

// Inserts a new start node into a specific position, pos
void KateCodeFoldingTree::insertStartNode(int type, KateDocumentPosition pos, int virtualColumn)
{
  // step 0 - set newNode's parameters
  // find the parent of the new node
  KateCodeFoldingNode *parentNode = findParent(pos,type);
  KateCodeFoldingNode *newNode = new KateCodeFoldingNode(parentNode,type,pos);
  newNode->m_virtualColumn = virtualColumn;

  // step 1 - divide parent's startChildrenList
  QVector <KateCodeFoldingNode*> tempList(parentNode->m_startChildren);
  sublist(parentNode->m_startChildren,tempList,KateDocumentPosition(0,0),newNode->m_position);
  sublist(newNode->m_startChildren,tempList,newNode->m_position,INFposition);

  // step 2 - divide parent's endChildrenList (or inherit shortage)
  if (parentNode->m_shortage > 0 && parentNode->m_type) {
    newNode->m_shortage = parentNode->m_shortage;
  }
  else {
    tempList = parentNode->m_endChildren;
    sublist(newNode->m_endChildren,tempList,newNode->m_position,INFposition);
    foreach (KateCodeFoldingNode *child, newNode->m_endChildren) {
      parentNode->removeEndDescending(child);
      parentNode->removeEndAscending(child);
    }
  }

  // step 3 - insert new node into map and add it to it's parent
  insertNodeIntoMap(newNode);
  parentNode->addChild(newNode);


  // step 4 - set the new parent for all the children
  newNode->setParent();
  parentNode->setParent();

  // step 5 - call updateParent te recalibrate tree
  newNode->updateSelf();
}

// called when a line has been inserted (key "Enter/Return" was pressed)
void KateCodeFoldingTree::lineHasBeenInserted(int line, int column)
{
  QMap <int, QVector <KateCodeFoldingNode*> > tempMap = m_lineMapping;
  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(tempMap);
  QVector <KateCodeFoldingNode*> tempVector;
  QVector <KateCodeFoldingNode*> tempVector2;
  m_lineMapping.clear();

  // Coppy the lines before "line"
  while (iterator.hasNext() && iterator.peekNext().key() < line) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    m_lineMapping.insert(key,tempVector);
    iterator.next();
  }

  // The nodes placed on "line" will be split usig the argument "column"
  while (iterator.hasNext() && iterator.peekNext().key() == line) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    // The nodes are split
    while (tempVector.empty() == false && tempVector[0]->getColumn() < column) {
      tempVector2.append(tempVector[0]);
      tempVector.pop_front();
    }
    incrementBy1(tempVector);
    m_lineMapping.insert(key, tempVector2);
    m_lineMapping.insert(key + 1,tempVector);
    iterator.next();
  }

  // All the other lines has to be updated before moved
  while (iterator.hasNext()) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    incrementBy1(tempVector);
    m_lineMapping.insert(key + 1,tempVector);
    iterator.next();
  }
}

// Called when a line was removed from the document
void KateCodeFoldingTree::linesHaveBeenRemoved(int from, int to)
{
  QMap <int, QVector <KateCodeFoldingNode*> > tempMap = m_lineMapping;
  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(tempMap);
  QVector <KateCodeFoldingNode*> tempVector;
  m_lineMapping.clear();

  // Coppy the lines before "line"
  while (iterator.hasNext() && iterator.peekNext().key() < from) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    m_lineMapping.insert(key,tempVector);
    iterator.next();
  }

  //
  while (iterator.hasNext() && iterator.peekNext().key() <= to) {
    // Coppy the old line
    tempVector = iterator.peekNext().value();
    int line = iterator.peekNext().key();
    m_lineMapping.insert(line,tempVector);

    // And remove all the nodes from it
    while (tempVector.isEmpty() == false) {
      deleteNode(tempVector.last());
      tempVector = m_lineMapping[line];
    }

    // If the deleted line was replaced with an empty line, the map entry will be deleted
    if (m_lineMapping[line].empty()) {
      m_lineMapping.remove(line);
    }

    iterator.next();
  }

  // All the other lines has to be updated before moved
  while (iterator.hasNext()) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    //decrementBy1(tempVector);
    addDeltaToLine(tempVector, (from - to - 1));
    m_lineMapping.insert(key + (from - to - 1),tempVector);
    iterator.next();
  }
}

// Update columns when "colsChanged" flag from updateLine() is "true"
// newColumns[2 * k + 1] = position of node k
void KateCodeFoldingTree::setColumns(int line, QVector<int> &newColumns, int virtualNodeIndex, int virtualColumn)
{
  QVector<KateCodeFoldingNode*> oldLineMapping = m_lineMapping[line];

  int index = 1;
  foreach (KateCodeFoldingNode* tempNode, oldLineMapping) {
    if (tempNode->m_type > 0 && index == virtualNodeIndex) {
      tempNode->m_virtualColumn = newColumns[index];
      tempNode->setColumn(virtualColumn);
    }
    else {
      tempNode->m_virtualColumn = newColumns[index];
      tempNode->setColumn(newColumns[index]);
    }
    index += 2;
  }
}

// puts in dest Vector a sublist of source Vector
void KateCodeFoldingTree::sublist(QVector<KateCodeFoldingNode *> &dest, QVector<KateCodeFoldingNode *> &source,
                                          KateDocumentPosition begin, KateDocumentPosition end)
{
  dest.clear();
  foreach (KateCodeFoldingNode *node, source) {
    if ((node->m_position >= end) && end != INFposition)
      break;
    if (node->m_position >= begin) {
      dest.append(node);
    }
  }
}

void KateCodeFoldingTree::foldNode(KateCodeFoldingNode *node)
{
  // We have to make sure that the lines below our folded node were parsed
  // We don't have to parse the entire file. We can stop when we find the match for our folded node
  for (int index = node->getLine() ; index < m_rootMatch->getLine() ; ++index) {
    m_buffer->ensureHighlighted(index);
    if (m_lineMapping.contains(index)) {
      QVector <KateCodeFoldingNode *> tempLineMap(m_lineMapping[index]);
      bool found = false;
      foreach (KateCodeFoldingNode* endNode, tempLineMap) {
        if (node->getStartMatching(endNode) == node) {
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
  }

  // Find out if there is another folded area contained by this new folded area
  // Ignore the included area (will not be inserted in hiddenNodes)
  QList <KateCodeFoldingNode*> oldHiddenNodes(m_hiddenNodes);
  m_hiddenNodes.clear();

  KateCodeFoldingNode* matchNode = node->matchingNode();
  if (!matchNode)
    matchNode = m_rootMatch;
  bool inserted = false;
  node->m_visible = false;

  foreach(KateCodeFoldingNode* tempNode, oldHiddenNodes) {
    KateCodeFoldingNode *tempMatch = tempNode->matchingNode();
    if (!tempMatch)
      tempMatch = m_rootMatch;

    // This folded area is above the new folded area
    // and it remains
    if (tempMatch->m_position < node->m_position) {
      m_hiddenNodes.append(tempNode);
    }

    // The new folded area is a subarea of this folded area,
    // We won't insert the new area anymore
    if (tempNode->m_position < node->m_position &&
        tempMatch->m_position > node->m_position) {
        m_hiddenNodes.append(tempNode);
        inserted = true;
    }

    // This folded area is a subarea of the new folded area
    // This area will not be copied
    else if (tempNode->getLine() >= node->getLine() &&
        tempMatch->getLine() <= matchNode->getLine() && node != tempNode) {
      if (inserted == false) {
        m_hiddenNodes.append(node);
        inserted = true;
      }
    }

    // This folded area is below the new folded area
    // and it remains
    else if (tempNode->getLine() >= matchNode->getLine()) {

      // If the current node was not inserted, now it's the time
      if (!inserted) {
        m_hiddenNodes.append(node);
        inserted = true;
      }
      m_hiddenNodes.append(tempNode);
    }
  }

  if (!inserted) {
    m_hiddenNodes.append(node);
  }

  oldHiddenNodes.clear();

  emit regionVisibilityChanged ();

  //printMapping();
}

void KateCodeFoldingTree::unfoldNode(KateCodeFoldingNode *node)
{
  QList <KateCodeFoldingNode*> newFoldedNodes;

  KateCodeFoldingNode* matchNode = node->matchingNode();
  if (!matchNode)
    matchNode = m_rootMatch;
  node->m_visible = true;

  KateDocumentPosition startPos = node->m_position;
  KateDocumentPosition endPos = matchNode->m_position;

  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(m_lineMapping);
  while (iterator.hasNext()) {
    int key = iterator.peekNext().key();

    // We might have some candidate nodes on this line
    if (key >= startPos.line && key <= endPos.line) {
      QVector <KateCodeFoldingNode*> tempLineMap = iterator.peekNext().value();

      foreach (KateCodeFoldingNode* tempNode, tempLineMap) {

        // End nodes and visible nodes are not interesting
        if (tempNode->m_type < 0 || tempNode->isVisible())
          continue;

        KateCodeFoldingNode* tempMatchNode = tempNode->matchingNode();
        if (!tempMatchNode)
          tempMatchNode = m_rootMatch;
        // this is a new folded area
        if (startPos < tempNode->m_position && endPos >= tempMatchNode->m_position) {
          newFoldedNodes.append(tempNode);

          // We update the start position (we don't want to search through its children
          startPos = tempMatchNode->m_position;

          // The matching node cannot be on the same line; we skip the rest of the line
          break;
        }
      }
    }

    iterator.next();
  }

  replaceFoldedNodeWithList(node, newFoldedNodes);

  emit regionVisibilityChanged();
}

// The method replace a single node with a list of nodes into the m_hiddenNodes data strucutre
void KateCodeFoldingTree::replaceFoldedNodeWithList(KateCodeFoldingNode *node, QList<KateCodeFoldingNode *> &newFoldedNodes)
{
  QList <KateCodeFoldingNode*> oldHiddenNodes(m_hiddenNodes);
  m_hiddenNodes.clear();
  bool inserted = false;

  foreach(KateCodeFoldingNode* tempNode, oldHiddenNodes) {
    // We copy this node
    if (tempNode->m_position < node->m_position || inserted) {
      m_hiddenNodes.append(tempNode);
    }

    // We skip this node (it is the node we'll replace)
    else if (tempNode->m_position == node->m_position)
      continue;

    else {
      // We copy the new list (if it was not yet copied)
      m_hiddenNodes.append(newFoldedNodes);
      inserted = true;

      // And we append the current node
      m_hiddenNodes.append(tempNode);
    }
  }

  if (inserted == false) {
    m_hiddenNodes.append(newFoldedNodes);
  }
}

// This method is called when the fold/unfold icon is pressed
void KateCodeFoldingTree::toggleRegionVisibility(int l)
{
  //KateCodeFoldingNode *tempNode = findNodeForLine(l);
  // If the line don't have any nodes, that we can't fold/unfold anything
  if (!m_lineMapping.contains(l))
    return;

  bool foldedFound = false;

  foreach (KateCodeFoldingNode* node, m_lineMapping[l]) {

    // We can fold/unfold only start nodes
    if (node->m_type < 0)
      continue;

    // If there are any folded nodes, they have priority
    if (!node->isVisible()) {
      unfoldNode(node);
      foldedFound = true;
    }
  }

  // If there were no folded nodes, we'll folde the first node
  if (!foldedFound) {
    foreach (KateCodeFoldingNode* node, m_lineMapping[l]) {

      // We can fold/unfold only start nodes
      if (node->m_type < 0)
        continue;

      // We fold the first unfolded node and exit
      if (node->isVisible()) {
        foldNode(node);
        break;
      }
    }
  }
}

// changed = true, if there is there is a new node on the line / a node was deleted from the line
// colschanged = true only if node's column changes (nodes de not appear/disappear)
void KateCodeFoldingTree::updateLine(int line, QVector<int> *regionChanges, bool *updated, bool changed, bool colsChanged)
{
  if (!changed && !colsChanged) {
    *updated = false;
    return;
  }

  int virtualIndex = hasVirtualColumns(*regionChanges);
  int virtualColumn = 0;
  if (virtualIndex > -1)
    virtualColumn = (*regionChanges)[virtualIndex - 1];

  if (colsChanged) {
    setColumns(line, *regionChanges, virtualIndex, virtualColumn);
    *updated = true;
    return;
  }

  // changed == true
  updateMapping(line, *regionChanges, virtualIndex, virtualColumn);
  *updated = true;
}

int KateCodeFoldingTree::hasVirtualColumns(QVector<int> &newColumns)
{
  for (int i = 2 ; i < newColumns.size() ; i += 2) {
    if (newColumns[i - 2] < 0 && newColumns[i] > 0 && newColumns[i - 1] > newColumns[i + 1])
      return i;
  }
  return -1;
}

// Update mapping when "changhed" flag from updateLine() is "true" - nodes inserted or deleted
// newColumns[2 * k + 1] = position of node k
void KateCodeFoldingTree::updateMapping(int line, QVector<int> &newColumns, int virtualNodeIndex, int virtualColumn)
{
  QVector<KateCodeFoldingNode*> oldLineMapping = m_lineMapping[line];
  int index_old = 0;
  int index_new = 1;

  // It works like a merge. It stops when we have no more new nodes
  for ( ; index_new < newColumns.size() ; index_new += 2 ) {

    // If the old mapping has ended, but we still got new nodes,
    // then a/some new node(s) was/were inserted
    if (index_old >= oldLineMapping.size()) {
      int nodeType = newColumns[index_new - 1];
      int nodeColumn = newColumns[index_new];
      if (nodeType < 0) {
        insertNode(nodeType, KateDocumentPosition(line,nodeColumn - 1), 0);
      }
      else {
        if (virtualNodeIndex == index_new - 1) {
          insertNode(nodeType, KateDocumentPosition(line,virtualColumn), nodeColumn);
          }
        else {
          insertNode(nodeType, KateDocumentPosition(line,nodeColumn), nodeColumn);
        }
      }
    }

    // If the nodes compared have the same type,
    // then we update the column (maybe the column has changed).
    else if (oldLineMapping[index_old]->m_type == newColumns[index_new - 1]) {
      // I can simply change the column only if this change will not mess the order of the line
      if ((index_old == (oldLineMapping.size() - 1)) || oldLineMapping[index_old + 1]->getColumn() >= newColumns[index_new])
      {
        if (newColumns[index_new - 1] < -1) {
          oldLineMapping[index_old]->setColumn(newColumns[index_new] - 1);
        }
        else  {
          oldLineMapping[index_old]->setColumn(newColumns[index_new]);
        }
      }
      // If the change of the column will change the nodes hierarchy, then we delete it
      else {
        deleteNode(oldLineMapping[index_old]);
        index_new -= 2;
      }
      index_old ++;
    }

    // If nodes ar different, then a node was inserted or deleted.
    // We assume that it was deleted. If we were wrong, it will be inserted lately.
    else {
      deleteNode(oldLineMapping[index_old]);
      index_new -= 2;
      index_old ++;
    }
  }

  // This are the nodes remaining. They do not exist in the new mapping and will be deleted
  for ( ; index_old < oldLineMapping.size() ; index_old ++) {
    deleteNode(oldLineMapping[index_old]);
  }

  // We check if the line is empty. If it is empty, we remove its entry
  if (m_lineMapping[line].isEmpty()) {
    m_lineMapping.remove(line);
  }

  oldLineMapping.clear();
}


void KateCodeFoldingTree::writeSessionConfig(KConfigGroup &config)
{
  saveFoldingState();
  config.writeEntry("FoldedLines", m_hiddenLines);
  config.writeEntry("FoldedColumns", m_hiddenColumns);
}

void KateCodeFoldingTree::readSessionConfig(const KConfigGroup &config)
{
  m_hiddenLines = config.readEntry("FoldedLines", QList<int>());
  m_hiddenColumns = config.readEntry("FoldedColumns", QList<int>());
  applyFoldingState();
}

void KateCodeFoldingTree::saveFoldingState()
{
  m_hiddenLines.clear();
  m_hiddenColumns.clear();

  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(m_lineMapping);
  while (iterator.hasNext()) {
    QVector <KateCodeFoldingNode*> tempVector = iterator.peekNext().value();
    foreach (KateCodeFoldingNode *node, tempVector) {
      if (!node->isVisible()) {
        m_hiddenLines.append(node->getLine());
        m_hiddenColumns.append(node->getColumn());
      }
    }
    iterator.next();
  }
}

void KateCodeFoldingTree::applyFoldingState()
{
  if (m_hiddenLines.isEmpty())
    return;

  // We have to make sure that the document was hl
  for (int index = 0 ; index < m_rootMatch->getLine() ; ++index) {
    m_buffer->ensureHighlighted(index);
  }

  QListIterator<int> itLines (m_hiddenLines);
  QListIterator<int> itColumns (m_hiddenColumns);
  while (itLines.hasNext()) {
    int line = itLines.next();
    int column = itColumns.next();

    QVector <KateCodeFoldingNode*> lineMap = m_lineMapping[line];
    foreach (KateCodeFoldingNode* node, lineMap) {
      if (node->getColumn() == column) {
        foldNode(node);
        break;
      }
    }
  }
}



// Debug methods

void KateCodeFoldingTree::printMapping() {
  debug() << "***************** New Line mapping print ******************";
  QMapIterator <int, QVector <KateCodeFoldingNode*> > iterator(m_lineMapping);
  while (iterator.hasNext()) {
    QVector <KateCodeFoldingNode*> tempVector = iterator.peekNext().value();
    int key = iterator.next().key();
    debug() << "Line" << key;
    foreach (KateCodeFoldingNode *node, tempVector) {
      debug() << "node type:" << node->m_type << ", col:" << node->getColumn()
              << ", line:" << node->getLine() << ", address: " << node;
      /*if (node->m_type > 0) {
        debug () << "start children...";
        foreach (KateCodeFoldingNode *child, node->m_startChildren) {
          debug() << "new start child address: " << child << "(line = " << child->getLine() <<")";
        }
        debug () << "end children...";
        foreach (KateCodeFoldingNode *child, node->m_endChildren) {
          debug() << "new end child address: " << child << "(line = " << child->getLine() <<")";
        }
      }*/
    }
    debug() << "End of line:" << key;
  }
  debug() << "**********************************************************";
}

void KateCodeFoldingTree::buildTreeString(KateCodeFoldingNode *node, int level)
{
  if (node->m_type == 0)
    treeString.clear();
  else
    treeString.append("\n");
  for (int i = 0 ; i < level-1 ; ++ i)
    treeString.append(QString("   "));

  if (node->m_type > 0)
  {
    treeString.append(QString("{ (l=%1, c=%2, vC=%3, pL=%4, pC=%5)").arg(node->getLine()).
                            arg(node->getColumn()).arg(node->m_virtualColumn).arg(node->m_parentNode->getLine()).
                            arg(node->m_parentNode->getColumn()));
  }
  else if (node->m_type < 0)
  {
    treeString.append(QString("} (l=%1, c=%2, vC=%3, pL=%4, pC=%5)").arg(node->getLine()).
                            arg(node->getColumn()).arg(node->m_virtualColumn).arg(node->m_parentNode->getLine()).
                            arg(node->m_parentNode->getColumn()));
  }

  int i1,i2;
  for (i1 = 0, i2 = 0 ; i1 < node->startChildrenCount() && i2 < node->m_endChildren.size() ;) {
    if (node->startChildAt(i1)->m_position < node->endChildAt(i2)->m_position) {
      buildTreeString(node->startChildAt(i1),level + 1);
      ++i1;
    }
    else {
      if (node->isDuplicated(node->endChildAt(i2)) == false)
        buildTreeString(node->endChildAt(i2),level);
      ++i2;
    }
  }

  for (; i1 < node->startChildrenCount() && i1 < node->startChildrenCount() ; ++ i1) {
    buildTreeString(node->startChildAt(i1),level + 1);
  }
  for (; i2 < node->m_endChildren.size() && i2 < node->m_endChildren.size() ; ++ i2) {
    if (node->isDuplicated(node->endChildAt(i2)) == false)
      buildTreeString(node->endChildAt(i2),level);
  }
}

void KateCodeFoldingTree::searchThisNode(KateCodeFoldingNode *deletedNode)
{
  QVector <KateCodeFoldingNode*> lineMap;
  QMessageBox msg;
  msg.setText("ERROR!!!!!!!");
  foreach (lineMap, m_lineMapping) {
    foreach (KateCodeFoldingNode* node, lineMap) {
      if (node == deletedNode) {
        msg.exec();
      }
      if (node->m_endChildren.contains(deletedNode)) {
        msg.exec();
      }
      if (node->m_startChildren.contains(deletedNode)) {
        msg.exec();
      }
    }
  }
}
