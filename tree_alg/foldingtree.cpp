#include "foldingtree.h"
#include "qdebug.h"

int INF = 1000;

int FoldingTree::newNodePos = -1;
bool FoldingTree::displayDetails = true;
bool FoldingTree::displayChildrenDetails = true;
bool FoldingTree::displayDuplicates = true;
QVector<QString> FoldingTree::history;
QVector<QString> FoldingTree::movesHistory;
int FoldingTree::nOps;

FoldingNode::FoldingNode(int pos, int typ, FoldingNode *par) :
    position(pos),
    parent(par),
    type(typ),
    shortage(1)
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
    add(node,m_startChildren);
  else
    add(node,m_endChildren);
}

FoldingNode* FoldingNode::removeChild(FoldingNode *node)
{
  if (node->type > 0) {
    if (remove(node,m_startChildren))
      return node;
  }
  else {
    if (remove(node,m_endChildren))
      return node;
  }
  return NULL;
}

QVector<FoldingNode *> FoldingNode::removeChilrenInheritedFrom(FoldingNode *node)
{
  int index;
  QVector<FoldingNode *> tempList;
  for (index = 0 ; index < m_endChildren.size() ; ++index) {
    if (node->contains(m_endChildren[index])) {
      FoldingNode* tempNode = removeChild(m_endChildren[index]);
      if (tempNode != NULL)
        tempList.append(tempNode);
      index --;
    }
  }
  return tempList;
}

// All the children must be kept sorted (using their position)
// This method inserts the new child in a children list
void FoldingNode::add(FoldingNode *node, QVector<FoldingNode*> &m_childred)
{
  int i;
  for (i = 0 ; i < m_childred.size() ; ++i) {
    if (m_childred[i]->position > node->position)
      break;
  }
  m_childred.insert(i,node);
}

bool FoldingNode::remove(FoldingNode *node, QVector<FoldingNode *> &m_childred)
{
  bool found = false;
  int i;

  for (i = 0 ; i < m_childred.size() ; ++i) {
    if (m_childred[i]->position == node->position) {
      found = true;
      break;
    }
  }
  if (found) {
    m_childred.remove(i);
    if (m_childred.empty())
      shortage = 1;
    if (type > 0 && node->type < 0) {
      parent->remove(node,parent->m_endChildren);
    }
    return true;
  }
  return false;
}

// Merges two QVectors of children.
// The result is placed in list1. list2 it is not modified.
void FoldingNode::mergeChildren(QVector <FoldingNode*> &list1, QVector <FoldingNode*> &list2)
{
  QVector <FoldingNode*> mergedList;
  int index1 = 0, index2 = 0;
  while (index1 < list1.size() && index2 < list2.size()) {
    if (list1[index1]->position < list2[index2]->position) {
      mergedList.append(list1[index1]);
      ++index1;
    }
    else if (list1[index1]->position > list2[index2]->position){
      mergedList.append(list2[index2]);
      ++index2;
    }
    else {
      mergedList.append(list1[index1]);
      ++index1;
      ++index2;
    }
  }

  for (; index1 < list1.size() ; ++index1)
    mergedList.append(list1[index1]);
  for (; index2 < list2.size() ; ++index2)
    mergedList.append(list2[index2]);

  list1 = mergedList;
}

// This method recalirates the folding tree after a modification occurs.
void FoldingNode::updateSelf()
{
  QVector <FoldingNode *> excessList;
  if (m_endChildren.size() > 0) {                       // this node doesn't have shortage (some children may become its brothers)
    for (int i = 0 ; i < m_startChildren.size() ; ++i) {
      FoldingNode* child = m_startChildren[i];
      if (child->position > matchingNode()->position) { // if child is lower than parent's pair
        remove(child, m_startChildren);                 // this node is not it's child anymore.
        QVector <FoldingNode *> temptExcessList;
        temptExcessList = removeChilrenInheritedFrom(child);              // and all the children inhereted from him will be removed
        mergeChildren(excessList,temptExcessList);
        i --;                                           // then, go one step behind (because we removed 1 element)
        parent->addChild(child);                        // and the node selected becomes it's broter (same parent)
      }
    }
  }
  else {                                                // this node has a shortage (all his brothers becomes his children)
    for (int i = 0 ; i < parent->m_startChildren.size() && hasMatch() == 0 ; ++i) { // ?
      FoldingNode* child = parent->m_startChildren[i];  // it's brother is selected
      if (child->position > position) {                 // and if this brother is above the current node, then
        remove(child, parent->m_startChildren);         // this node is not it's brother anymore, but it's child
        i --;                                           // go one step behind (because we removed 1 element)
        addChild(child);                                // the node selected becomes it's child
        //
        QVector <FoldingNode*> tempList (child->m_endChildren);
        if (tempList.isEmpty() == false) {
          tempList.pop_front();                             // we take the excess of it's children
          mergeChildren(m_endChildren,tempList);            // and merge to it's endChildren list
        }
      }
    }
  }

  if (m_startChildren.size() > 0)                       // if the node has children, then all its children will be inhereted
    m_endChildren.clear();                              // we rebuild end_children list
  foreach (FoldingNode *child, m_startChildren) {
    if (child->shortage > 0) {                          // the only child this node has
      shortage = child->shortage + 1;
      break;
    }
    else {
      QVector <FoldingNode*> tempList (child->m_endChildren);
      tempList.pop_front();                             // we take the excess of it's children
      mergeChildren(m_endChildren,tempList);            // and merge to it's endChildren list
    }
  }
  setParent();
  if (hasMatch()) {
    shortage = 0;
    parent->removeChild(matchingNode());
  }

  // call updateParent();
  QVector<FoldingNode*> tempList (m_endChildren);
  if (!tempList.empty())
      tempList.pop_front();
  mergeChildren(tempList,excessList);
  parent->updateParent(tempList,shortage);
}

// This method helps recalibrating the tree
void FoldingNode::updateParent(QVector <FoldingNode *> newExcess, int newShortage)
{
  mergeChildren(m_endChildren,newExcess);               // parent's endChildren list is updated
  setParent();
  //m_endChildren = newExcess;
  if (newShortage > 0)
    shortage = newShortage + 1;                         // parent's shortage is updated
  else if (hasMatch())
    shortage = 0;
  else
    shortage = 1;
  if (type)                                             // if this node is not the root node
    updateSelf();                                       // then recalibration continues
}

bool FoldingNode::hasBrothers() {
  if (type == 0)
    return false;
  if (parent->m_startChildren.size() > 0)
    return true;
  return false;
}

// This method will ensure that all children have their parent set correct
void FoldingNode::setParent()
{
  foreach(FoldingNode *child, m_startChildren) {
    child->parent = this;
  }
  foreach(FoldingNode *child, m_endChildren) {
    bool change = true;
    if (isDuplicated(child) == true)
      change = false;
    /*else {
      if (hasBrothers())
        foreach (FoldingNode *brother, parent->m_startChildren)
          if (brother->position > position && brother->contains(child))
            change = false;
    }*/
    if (change)
      child->parent = this;
  }
}

bool FoldingNode::contains(FoldingNode *node) {
  if (node->type > 0) {
    foreach (FoldingNode *child, m_startChildren)
      if (child->position == node->position)
        return true;
    return false;
  }
  else if (node->type < 0) {
    foreach (FoldingNode *child, m_endChildren)
      if (child->position == node->position)
        return true;
    return false;
  }
  return false;
}

bool FoldingNode::isDuplicatedPrint(FoldingNode *node)
{
  /*foreach (FoldingNode *child, m_startChildren)
    if (child->contains(node))
      return true;
  return false;
  */
  if (type == 0)
    return false;
  return parent->contains(node);
}

bool FoldingNode::isDuplicated(FoldingNode *node)
{
  foreach (FoldingNode *child, m_startChildren)
    if (child->contains(node))
      return true;
  return false;
}

QString FoldingNode::toString(int level)
{
  QString string = "\n";
  for (int i = 0 ; i < level-1 ; ++ i)
    string.append(QString("    "));

  //qDebug()<<QString("list size = %1").arg(m_startChildren.size());
  if (FoldingTree::displayDetails) {
    if (type > 0)
    {
      string.append(QString("{ (POS=%1, pPos=%2, shortage=%3)").arg(position).arg(parent->position).arg(shortage));
      //qDebug()<<QString("!!!!!!!!!parent = %1").arg(parent->position);
    }
    else if (type < 0)
    {
      string.append(QString("  } (POS=%1, pPos=%2)").arg(position).arg(parent->position));
    }
  }
  else {
    if (type > 0) string.append("{");
    else if (type < 0) string.append("  }");
  }

  if (FoldingTree::displayChildrenDetails && type >= 0) {
    string.append(" startChPos:[");
    int i;
    for (i = 0 ; i < m_startChildren.size() ; ++ i) {
      string.append(QString("%1,").arg(m_startChildren[i]->position));
    }
    string.append("] endChPos:[");
    for (i = 0 ; i < m_endChildren.size() ; ++ i) {
      string.append(QString("%1,").arg(m_endChildren[i]->position));
    }
    string.append("]");
  }

  if (FoldingTree::newNodePos == position) {
    string.append(" - NEW");
  }

  int i1,i2;
  for (i1 = 0, i2 = 0 ; i1 < m_startChildren.size() && i2 < m_endChildren.size() ;) {
    if (m_startChildren[i1]->position < m_endChildren[i2]->position) {
      string.append(m_startChildren[i1]->toString(level + 1));
      ++i1;
    }
    else {
      if (FoldingTree::displayDuplicates == true || isDuplicatedPrint(m_endChildren[i2]) == false)
        string.append(m_endChildren[i2]->toString(level));
      ++i2;
    }
  }

  for (; i1 < m_startChildren.size() && i1 < m_startChildren.size() ; ++ i1) {
    string.append(m_startChildren[i1]->toString(level + 1));
  }
  for (; i2 < m_endChildren.size() && i2 < m_endChildren.size() ; ++ i2) {
    if (FoldingTree::displayDuplicates == true || isDuplicatedPrint(m_endChildren[i2]) == false)
      string.append(m_endChildren[i2]->toString(level));
  }

  return string;
}

FoldingTree::FoldingTree()
{
  root = new FoldingNode(0,0,NULL);
  nodeMap.clear();
  nodeMap.insert(0,root);
  newNodePos = -1;
}

FoldingTree::~FoldingTree()
{
}

// Search for the parent of the new node that will be created at position startingPos + 1
FoldingNode* FoldingTree::findParent(int startingPos,int childType)
{
  int i;
  for (i = (startingPos < nodeMap.size() - 1) ? startingPos : nodeMap.size() - 1 ; i > 0 ; -- i) {
    if (nodeMap[i]->type < 0)                                             // The parent must be a "start node"
      continue;
    if (childType < 0)                                                    // If it's inserted an "end node",
      return nodeMap[i];                                                  // then there are no other conditions
    if (nodeMap[i]->hasMatch() == false)                                  // if the node has a shortage, it can be a parent
      return nodeMap[i];
    if (nodeMap[i]->m_endChildren.last()->position > startingPos + 1)     // if this node has a match and its matching node's
      return nodeMap[i];                                                  // position is lower then current nide, it can be its parent
  }
  return root;                                                            // The root node is the default parent
}

FoldingNode* FoldingTree::fineNodeAbove(int startingPos)
{
  int i;
  for (i = (startingPos < nodeMap.size() - 1) ? startingPos : nodeMap.size() - 1 ; i > 0 ; -- i) {
    if (nodeMap[i]->type > 0)                                             // The parent must be a "start node"
      return nodeMap[i];
  }
  return root;                                                            // The root node is the default parent
}

void FoldingTree::insertStartNode(int pos)
{
  // step 0 - set newNode's parameters
  increasePosition(pos + 1);                                              // update the others nodes position (+ 1)
  FoldingNode *parentNode = findParent(pos,1);                            // find the parent of the new node
  FoldingNode *newNode = new
                FoldingNode(pos + 1,1,parentNode);                        // create the new node

  // debug
  newNodePos = newNode->position;
  // end of debug

  // step 1 - divide parent's startChildrenList
  QVector <FoldingNode*> tempList(parentNode->m_startChildren);
  sublist(parentNode->m_startChildren,tempList,0,newNode->position);
  sublist(newNode->m_startChildren,tempList,newNode->position,INF);

  // step 2 - divide parent's endChildrenList (or inherit shortage)
  if (parentNode->shortage > 0 && parentNode->type) {                     // parent is not root node
    newNode->shortage = parentNode->shortage;
  }
  else {
    tempList = parentNode->m_endChildren;
    //sublist(parentNode->m_endChildren,tempList,0,newNode->position);
    sublist(newNode->m_endChildren,tempList,newNode->position,INF);
    foreach (FoldingNode *child, newNode->m_endChildren) {
      parentNode->remove(child,parentNode->m_endChildren);
    }
  }

  // step 3 - insert new node into position
  nodeMap.insert(newNode->position,newNode);                              // insert it in the map
  parentNode->addChild(newNode);                                          // add the node to it's parent


  // step 4 - set the new parent for all the children
  newNode->setParent();
  parentNode->setParent();

  // step 5 - call updateParent te recalibrate tree
  /*tempList = newNode->m_endChildren;
  if (!tempList.empty())
    tempList.pop_front();
  newNode->parent->updateParent(tempList,newNode->shortage);*/
  newNode->updateSelf();

}

void FoldingTree::insertEndNode(int pos)
{
  // step 0 - set newNode's parameters
  FoldingNode *parentNode = findParent(pos,-1);                           // find the parent of the new node
  FoldingNode *newNode = new
               FoldingNode(pos + 1,-1,parentNode);                        // create the new node
  nodeMap.insert(newNode->position,newNode);                              // insert it in the map
  increasePosition(newNode->position + 1);                                // update the others nodes position (+ 1)
  parentNode->addChild(newNode);                                          // add the node to it's parent

  // debug
  newNodePos = newNode->position;
  // end of debug

  // step 1 - call updateSelf() for parent node to recalibrate tree
  if (parentNode->type)                                                   // if its parent is the root node, then it stops
    parentNode->updateSelf();
}

void FoldingTree::deleteStartNode(int pos)
{
  // step 0 - find the node that will be deleted
  if (pos >= nodeMap.size())
    return;
  if (pos <= 0)
    return;
  FoldingNode *deletedNode = findNodeAt(pos);
  nodeMap.remove(pos);

  // step 1 - all its startChildren are inherited by its parent
  //deletedNode->parent->mergeChildren(deletedNode->parent->m_startChildren,deletedNode->m_startChildren);
  FoldingNode *heir = fineNodeAbove(deletedNode->position - 1);
  heir->mergeChildren(heir->m_startChildren,deletedNode->m_startChildren);

  // step 2 - this node is removed from the tree
  deletedNode->parent->removeChild(deletedNode);
  decreasePosition(deletedNode->position);

  // step 3 - parent inherits shortage and endChildren too
  heir->updateParent(deletedNode->m_endChildren,deletedNode->shortage - 1);

  // debug
  newNodePos = -1;
  // end of debug

  // step 4 - node is deleted
  delete deletedNode;
}

void FoldingTree::deleteEndNode(int pos)
{
  // step 0 - find the node that will be deleted
  if (pos >= nodeMap.size())
    return;
  if (pos <= 0)
    return;
  FoldingNode *deletedNode = findNodeAt(pos);
  nodeMap.remove(pos);

  // step 1 - this node is removed from the tree
  deletedNode->parent->removeChild(deletedNode);
  decreasePosition(deletedNode->position);

  // step 2 - recalibrate folding tree starting from parent
  if (deletedNode->parent->type)
    deletedNode->parent->updateSelf();

  // debug
  newNodePos = -1;
  // end of debug

  // step 3 - node is deleted
  delete deletedNode;
}

void FoldingTree::deleteNode(int pos)
{
  // step 0 - find the node that will be deleted
  if (pos >= nodeMap.size())
    return;
  if (pos <= 0)
    return;
  FoldingNode *deletedNode = findNodeAt(pos);
  if (deletedNode->type > 0) {
    deleteStartNode(pos);
  }
  else {
    deleteEndNode(pos);
  }
}

// Increase position for each node placed after the inserted node
void FoldingTree::increasePosition(int startingPos)
{
  for (int i = startingPos ; i < nodeMap.size(); ++i) {
    nodeMap[i]->position ++;
  }
}

// Decrease position for each node placed after the inserted node
void FoldingTree::decreasePosition(int startingPos)
{
  for (int i = startingPos ; i < nodeMap.size(); ++i) {
    nodeMap[i]->position --;
  }
}

FoldingNode* FoldingTree::findNodeAt(int position) {
  foreach (FoldingNode *node,nodeMap) {
    if (node->position == position)
      return node;
  }
  return NULL;
}

// puts in dest Vector a sublist of source Vector
void FoldingTree::sublist(QVector<FoldingNode *> &dest, QVector<FoldingNode *> source, int begin, int end)
{
  dest.clear();
  foreach (FoldingNode *node, source) {
    if (node->position >= end)
      break;
    if (node->position >= begin) {
      dest.append(node);
    }
  }
}

QString FoldingTree::toString()
{
  /*qDebug()<<QString("\n********\nThe node's map:\n");
  for (int i = 0 ; i < nodeMap.size() ; i++) {
    qDebug()<<QString("(%1 ; %2)").arg(nodeMap[i]->position).arg(nodeMap[i]->type);
  }*/
  return "(0)" + root->toString();
}

void FoldingTree::buildStackString()
{
  QStack<FoldingNode *> testStack;
  stackString.clear();
  testStack.clear();
  int level = 0;
  int index = -1;
  int nPops = 0;

  testStack.push(nodeMap[0]);
  foreach (FoldingNode *node, nodeMap) {
    index ++;
    int tempPops = 0;
    if (node->type > 0) {
      if (nPops) {
        tempPops = nPops;
        while (tempPops && testStack.top()->type) {
          testStack.pop();
          tempPops --;
        }
        level -= nPops;
        if (level < 1)
          level = 0;
        nPops = 0;
      }
      level ++;
      stackString.append("\n");
      for (int i = 0 ; i < level ; ++ i)
        stackString.append(QString("   "));
      stackString.append(QString("{ (POS=%1, pPos=%2)").arg(node->position).arg(testStack.top()->position));
      testStack.push(node);
    }
    else if (node->type < 0) {
      stackString.append("\n");
      for (int i = 0 ; i < level ; ++ i)
        stackString.append(QString("   "));
      stackString.append(QString("} (POS=%1, pPos=%2)").arg(node->position).arg(testStack.top()->position));
      nPops ++;
    }
  }
}

void FoldingTree::buildTreeString(FoldingNode *node, int level)
{
  if (node->type == 0)
    treeString.clear();
  else
    treeString.append("\n");
  for (int i = 0 ; i < level-1 ; ++ i)
    treeString.append(QString("   "));

  if (node->type > 0)
  {
    treeString.append(QString("{ (POS=%1, pPos=%2)").arg(node->position).arg(node->parent->position));
  }
  else if (node->type < 0)
  {
    treeString.append(QString("} (POS=%1, pPos=%2)").arg(node->position).arg(node->parent->position));
  }

  int i1,i2;
  for (i1 = 0, i2 = 0 ; i1 < node->m_startChildren.size() && i2 < node->m_endChildren.size() ;) {
    if (node->m_startChildren[i1]->position < node->m_endChildren[i2]->position) {
      buildTreeString(node->m_startChildren[i1],level + 1);
      ++i1;
    }
    else {
      if (node->isDuplicated(node->m_endChildren[i2]) == false)
        buildTreeString(node->m_endChildren[i2],level);
      ++i2;
    }
  }

  for (; i1 < node->m_startChildren.size() && i1 < node->m_startChildren.size() ; ++ i1) {
    buildTreeString(node->m_startChildren[i1],level + 1);
  }
  for (; i2 < node->m_endChildren.size() && i2 < node->m_endChildren.size() ; ++ i2) {
    if (node->isDuplicated(node->m_endChildren[i2]) == false)
      buildTreeString(node->m_endChildren[i2],level);
  }
}

bool FoldingTree::isCorrect()
{
  return  (stackString.compare(treeString) == 0 ? true : false);
}
