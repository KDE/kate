#include "foldingtree.h"
#include "qdebug.h"

FoldingNode::FoldingNode(int no, int typ, FoldingNode *par) :
    nodeNo(no),
    parent(par),
    type(typ)
{
}

FoldingNode::~FoldingNode()
{
  m_startChildren.clear();
  m_endChildren.clear();
}

FoldingNode* FoldingNode::matchingNode()
{
  if (m_endChildren.size() > 0)
    return m_endChildren[0];
  return NULL;
}

void FoldingNode::addChild(FoldingNode *node)
{
  if (node->type > 0)
    insert(node,m_startChildren);
  else
    insert(node,m_endChildren);
}

void FoldingNode::deleteChild(FoldingNode *node)
{
  if (node->type > 0)
    remove(node,m_startChildren);
  else
    remove(node,m_endChildren);
}

// All the children must be kept sorted (using their position)
// This method inserts the new child
void FoldingNode::insert(FoldingNode *node, QVector<FoldingNode*> &m_childred)
{
  int i;
  for (i = 0 ; i < m_childred.size() ; ++i) {
    if (m_childred[i]->nodeNo > node->nodeNo)
      break;
  }
  m_childred.insert(i,node);
}

void FoldingNode::remove(FoldingNode *node, QVector<FoldingNode *> &m_childred)
{
  int i;
  for (i = 0 ; i < m_childred.size() ; ++i) {
    if (m_childred[i]->nodeNo == node->nodeNo)
      break;
  }
  m_childred.remove(i);
}

// Merges two QVectors of children.
// The result is placed in list1. list2 it is not modified.
void FoldingNode::mergeChildren(QVector <FoldingNode*> &list1, QVector <FoldingNode*> &list2)
{
  QVector <FoldingNode*> mergedList;
  int index1 = 0, index2 = 0;
  while (index1 < list1.size() && index2 < list2.size()) {
    if (list1[index1]->nodeNo < list2[index2]->nodeNo) {
      mergedList.append(list1[index1]);
      ++index1;
    }
    else {
      mergedList.append(list2[index2]);
      ++index2;
    }
  }

  for (; index1 < list1.size() ; ++index1)
    mergedList.append(list1[index1]);
  for (; index2 < list2.size() ; ++index2)
    mergedList.append(list2[index2]);

  list1 = mergedList;
}

// This method recalirates the folding tree after a moddification occurs.
void FoldingNode::updateSelf()
{
  if (m_endChildren.size() > 0) {                   // this node doesn't have shortage
    for (int i = 0 ; i < m_startChildren.size() ; ++i) {
      FoldingNode* child = m_startChildren[i];
      if (child->nodeNo > matchingNode()->nodeNo) { // if child is lower than parent's pair
        remove(child, m_startChildren);             // this node is not it's child anymore
        i --;                                       // go one step behind (because we removed 1 element)
        parent->addChild(child);                    // the node selected becomes it's broter (same parent)
      }
    }
    m_endChildren.clear();                          // we rebuild end_children list
    foreach (FoldingNode *child, m_startChildren) {
      if (child->shortage > 0) {                    // the only child this node has
        shortage = child->shortage + 1;
      }
      else {
        QVector <FoldingNode*> tempList (child->m_endChildren);
        tempList.pop_front();                       // we take the excess of it's children
        mergeChildren(m_endChildren,tempList);      // and merge to it's endChildren list
      }
    }
  }
}

// This method helps recalibrating the tree
void FoldingNode::updateParent(QVector <FoldingNode *> newExcess, int newShortage)
{
  mergeChildren(m_endChildren,newExcess);           // parent's endChildren list is updated
  if (newShortage > 0)
    shortage = newShortage + 1;                     // parent's shortage is updated
  if (type)                                         // if this node is not the root node
    updateSelf();                                   // then recalibration continues
}

QString FoldingNode::toString(int level)
{
  QString string = "\n";
  for (int i = 0 ; i < level-1 ; ++ i)
    string.append(QString("   "));

  //qDebug()<<QString("list size = %1").arg(m_startChildren.size());
  if (type > 0)
  {
    string.append(QString("{ (nodeNo=%1, parentNo=%2)").arg(nodeNo).arg(parent->nodeNo));
    //qDebug()<<QString("!!!!!!!!!parent = %1").arg(parent->nodeNo);
  }
  else if (type < 0)
  {
    string.append(QString("} (nodeNo=%1, parentNo=%2)").arg(nodeNo).arg(parent->nodeNo));
  }
  //qDebug()<<QString("\n------\nnew string = %1\n--------\n").arg(string);
  int i1,i2;
  for (i1 = 0, i2 = 0 ; i1 < m_startChildren.size() && i2 < m_endChildren.size() ;) {
    if (m_startChildren[i1]->nodeNo < m_endChildren[i2]->nodeNo) {
      string.append(m_startChildren[i1]->toString(level + 1));
      ++i1;
    }
    else {
      string.append(m_endChildren[i2]->toString(level));
      ++i2;
    }
  }

  for (; i1 < m_startChildren.size() && i1 < m_startChildren.size() ; ++ i1) {
    string.append(m_startChildren[i1]->toString(level + 1));
  }
  for (; i2 < m_endChildren.size() && i2 < m_endChildren.size() ; ++ i2) {
    string.append(m_endChildren[i2]->toString(level + 1));
  }
  return string;
}

FoldingTree::FoldingTree()
{
  root = new FoldingNode(0,0,NULL);
  nodeMap.clear();
  nodeMap.insert(0,root);
}

FoldingTree::~FoldingTree()
{
}

void FoldingTree::insertStartNode(int pos)
{
  FoldingNode *parentNode = findParent(pos);                    // find the parent of the new node
  FoldingNode *newNode = new FoldingNode(pos+1,1,parentNode);   // create the new node
  nodeMap.insert(pos+1,newNode);                                // insert it in the map
  nodeInserted(pos + 2);                                        // update the others nodes position
  parentNode->addChild(newNode);                                // add the node to it's parent
}

void FoldingTree::insertEndNode(int pos)
{
  FoldingNode* parentNode = findParent(pos);
  FoldingNode* newNode = new FoldingNode(pos+1,-1,parentNode);
  nodeMap.insert(pos+1,newNode);
  nodeInserted(pos + 2);
  parentNode->addChild(newNode);
}

void FoldingTree::deleteNode(int pos)
{
  if (pos >= nodeMap.size())
    return;
  if (pos <= 0)
    return;
  nodeMap[pos]->parent->deleteChild(nodeMap[pos]);
  root->remove(nodeMap[pos],nodeMap);
  nodeDeleted(pos);
}

// Search for the parent of the new node that will be created at position startingPos + 1
FoldingNode* FoldingTree::findParent(int startingPos) // Node placed at position startingPos is the first candidate
{
  int i;
  for (i = startingPos ; i > 0 ; -- i) {
    if (nodeMap[i]->type > 0)                         // The parent node has to be a "start node"
      return nodeMap[i];
  }
  return root;                                        // The root node is the default parent
}

// Increase nodeNo for each node placed after the inserted node
void FoldingTree::nodeInserted(int pos)
{
  for (int i = pos ; i < nodeMap.size(); ++i) {
    nodeMap[i]->nodeNo ++;
  }
}

void FoldingTree::nodeDeleted(int pos)
{
  for (int i = pos ; i < nodeMap.size(); ++i) {
    nodeMap[i]->nodeNo --;
  }
}

QString FoldingTree::toString()
{
  /*qDebug()<<QString("\n********\nThe node's map:\n");
  for (int i = 0 ; i < nodeMap.size() ; i++) {
    qDebug()<<QString("(%1 ; %2)").arg(nodeMap[i]->nodeNo).arg(nodeMap[i]->type);
  }*/
  return "(0)" + root->toString();
}
