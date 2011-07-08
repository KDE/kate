#include "katecodefoldingabstract.h"
#include "ktexteditor/cursor.h"

// Debug
#include "QtCore/QStack"
#include "QMessageBox"
//

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp() :
    parentNode(0),
    position(0,0),
    //startLineValid(false),
    type(0),
    visible(true),
    shortage(1)
{
}

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp(KateCodeFoldingNodeTemp *parent, signed char typ, KateDocumentPosition pos):
    parentNode(parent),
    position(pos),
    //startLineValid(true),
    type(typ),
    visible(true),
    shortage(1)
{
}

KateCodeFoldingNodeTemp::~KateCodeFoldingNodeTemp()
{
//  m_startChildren.clear();
//  m_endChildren.clear();
}

// All the children must be kept sorted (using their position)
// This method inserts the new child in a children list
void KateCodeFoldingNodeTemp::add(KateCodeFoldingNodeTemp *node, QVector<KateCodeFoldingNodeTemp*> &m_childred)
{
  int i;
  for (i = 0 ; i < m_childred.size() ; ++i) {
    if (m_childred[i]->position > node->position)
      break;
  }
  m_childred.insert(i,node);
}

void KateCodeFoldingNodeTemp::addChild(KateCodeFoldingNodeTemp *node)
{
  if (node->type > 0)
    add(node,m_startChildren);
  else
    add(node,m_endChildren);
}

bool KateCodeFoldingNodeTemp::contains(KateCodeFoldingNodeTemp *node) {
  if (node->type > 0) {
    foreach (KateCodeFoldingNodeTemp *child, m_startChildren)
      if (child->position == node->position)
        return true;
    return false;
  }
  else if (node->type < 0) {
    foreach (KateCodeFoldingNodeTemp *child, m_endChildren)
      if (child->position == node->position)
        return true;
    return false;
  }
  return false;
}

// The tree it's not used. It is inherited from the previuos implementation
// and kept for compatibility (at least for now...)
bool KateCodeFoldingNodeTemp::getBegin(AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor* begin) {
  //if (!startLineValid) return false;
  // It must be a valid start node
  if (type < 1) return false;

  begin->setLine(getLine());
  begin->setColumn(getColumn());

  return true;
}

int KateCodeFoldingNodeTemp::getDepth() {
  if (type == 0)
    return 0;
  return parentNode->getDepth() + 1;
}

bool KateCodeFoldingNodeTemp::getEnd(AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor *end) {
  if (type == 0) {
    end->setLine(tree->m_rootMatch->getLine());
    end->setColumn(tree->m_rootMatch->getColumn());
  }
  if ( ! hasMatch() ) return false;

  end->setLine(matchingNode()->getLine());
  end->setColumn(matchingNode()->getColumn() + 1);
  // We want to fold "}" too (+ 1)

  return true;
}

KateCodeFoldingNodeTemp* KateCodeFoldingNodeTemp::getStartMatching(KateCodeFoldingNodeTemp* endNode) {
  // If node's parent is root node, then we're done
  if (type == 0)
    return this;

  // Else
  if (matchingNode()->position == endNode->position)
    return this;

  // Else (it was an excess node)
  return parentNode->getStartMatching(endNode);
}

bool KateCodeFoldingNodeTemp::hasBrothers() {
  if (type == 0)
    return false;
  if (parentNode->m_startChildren.size() > 0)
    return true;
  return false;
}

bool KateCodeFoldingNodeTemp::isDuplicated(KateCodeFoldingNodeTemp *node)
{
  foreach (KateCodeFoldingNodeTemp *child, m_startChildren)
    if (child->contains(node))
      return true;
  return false;
}

// Merges two QVectors of children.
// The result is placed in list1. list2 it is not modified.
void KateCodeFoldingNodeTemp::mergeChildren(QVector <KateCodeFoldingNodeTemp*> &list1, QVector <KateCodeFoldingNodeTemp*> &list2)
{
  QVector <KateCodeFoldingNodeTemp*> mergedList;
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

bool KateCodeFoldingNodeTemp::remove(KateCodeFoldingNodeTemp *node, QVector<KateCodeFoldingNodeTemp *> &m_childred)
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
      parentNode->remove(node,parentNode->m_endChildren);
    }
    return true;
  }
  return false;
}

KateCodeFoldingNodeTemp* KateCodeFoldingNodeTemp::removeChild(KateCodeFoldingNodeTemp *node)
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

QVector<KateCodeFoldingNodeTemp *> KateCodeFoldingNodeTemp::removeChildrenInheritedFrom(KateCodeFoldingNodeTemp *node)
{
  int index;
  QVector<KateCodeFoldingNodeTemp *> tempList;
  for (index = 0 ; index < m_endChildren.size() ; ++index) {
    if (node->contains(m_endChildren[index])) {
      KateCodeFoldingNodeTemp* tempNode = removeChild(m_endChildren[index]);
      if (tempNode != NULL)
        tempList.append(tempNode);
      index --;
    }
  }
  return tempList;
}

/*
KateCodeFoldingNodeTemp *KateCodeFoldingNodeTemp::removeEndChild(int index)
{
  if (index >= m_endChildren.size ())
    return NULL;
  if (index < 0)
    return NULL;

  KateCodeFoldingNodeTemp *node = m_endChildren[index];
  m_endChildren.remove(index);

  return node;
}

KateCodeFoldingNodeTemp *KateCodeFoldingNodeTemp::removeStartChild(int index)
{
  if (index >= m_startChildren.size ())
    return NULL;
  if (index < 0)
    return NULL;

  KateCodeFoldingNodeTemp *node = m_startChildren[index];
  m_startChildren.remove(index);

  return node;
}
*/

// This method will ensure that all children have their parent set correct
void KateCodeFoldingNodeTemp::setParent()
{
  foreach(KateCodeFoldingNodeTemp *child, m_startChildren) {
    child->parentNode = this;
  }
  foreach(KateCodeFoldingNodeTemp *child, m_endChildren) {
    bool change = true;
    if (isDuplicated(child) == true)
      change = false;

    if (change)
      child->parentNode = this;
  }
}

// This method helps recalibrating the tree
void KateCodeFoldingNodeTemp::updateParent(QVector <KateCodeFoldingNodeTemp *> newExcess, int newShortage)
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

// This method recalirates the folding tree after a modification occurs.
void KateCodeFoldingNodeTemp::updateSelf()
{
  QVector <KateCodeFoldingNodeTemp *> excessList;
  if (m_endChildren.size() > 0) {                       // this node doesn't have shortage (some children may become its brothers)
    for (int i = 0 ; i < m_startChildren.size() ; ++i) {
      KateCodeFoldingNodeTemp* child = m_startChildren[i];
      if (child->position > matchingNode()->position) { // if child is lower than parent's pair
        remove(child, m_startChildren);                 // this node is not it's child anymore.
        QVector <KateCodeFoldingNodeTemp *> temptExcessList;
        temptExcessList = removeChildrenInheritedFrom(child);              // and all the children inhereted from him will be removed
        mergeChildren(excessList,temptExcessList);
        i --;                                           // then, go one step behind (because we removed 1 element)
        parentNode->addChild(child);                        // and the node selected becomes it's broter (same parent)
      }
    }
  }
  else {                                                // this node has a shortage (all his brothers becomes his children)
    for (int i = 0 ; i < parentNode->m_startChildren.size() && hasMatch() == 0 ; ++i) { // ?
      KateCodeFoldingNodeTemp* child = parentNode->m_startChildren[i];  // it's brother is selected
      if (child->position > position) {                 // and if this brother is above the current node, then
        remove(child, parentNode->m_startChildren);         // this node is not it's brother anymore, but it's child
        i --;                                           // go one step behind (because we removed 1 element)
        addChild(child);                                // the node selected becomes it's child
        //
        QVector <KateCodeFoldingNodeTemp*> tempList (child->m_endChildren);
        if (tempList.isEmpty() == false) {
          tempList.pop_front();                             // we take the excess of it's children
          mergeChildren(m_endChildren,tempList);            // and merge to it's endChildren list
        }
      }
    }
  }

  if (m_startChildren.size() > 0)                       // if the node has children, then all its children will be inhereted
    m_endChildren.clear();                              // we rebuild end_children list
  foreach (KateCodeFoldingNodeTemp *child, m_startChildren) {
    if (child->shortage > 0) {                          // the only child this node has
      shortage = child->shortage + 1;
      break;
    }
    else {
      QVector <KateCodeFoldingNodeTemp*> tempList (child->m_endChildren);
      tempList.pop_front();                             // we take the excess of it's children
      mergeChildren(m_endChildren,tempList);            // and merge to it's endChildren list
    }
  }
  setParent();
  if (hasMatch()) {
    shortage = 0;
    parentNode->removeChild(matchingNode());
  }

  // call updateParent();
  QVector<KateCodeFoldingNodeTemp*> tempList (m_endChildren);
  if (!tempList.empty())
      tempList.pop_front();
  mergeChildren(tempList,excessList);
  parentNode->updateParent(tempList,shortage);
}


// End of KateCodeFoldingNodeTemp Class

//-------------------------------------------------------//

// Start of AbstractKateCodeFoldingTree


AbstractKateCodeFoldingTree::AbstractKateCodeFoldingTree(KateBuffer *buffer) :
    m_buffer(buffer),
    INFposition(-10,10)
{
  m_root = new KateCodeFoldingNodeTemp(0,0,KateDocumentPosition(-1,-1));
  m_rootMatch = new KateCodeFoldingNodeTemp(0,0,KateDocumentPosition(0,0));
  m_lineMapping.clear();
  QVector<KateCodeFoldingNodeTemp *> tempVector;
  tempVector.append(m_root);
  m_lineMapping.insert(-1,tempVector);
}

AbstractKateCodeFoldingTree::~AbstractKateCodeFoldingTree()
{
}

void AbstractKateCodeFoldingTree::clear()
{
  QList<int>keys = m_lineMapping.uniqueKeys();
  foreach (int key, keys) {
    QVector<KateCodeFoldingNodeTemp*> tempLineMap = m_lineMapping[key];
    while (tempLineMap.empty() == false) {
      delete tempLineMap.last();
    }
  }
  m_lineMapping.clear();
  m_root->m_startChildren.clear();
  m_root->m_endChildren.clear();
}

int AbstractKateCodeFoldingTree::collapseOne(int realLine)
{
  qDebug()<<QString("colapse one ... at %1").arg(realLine);
}

void AbstractKateCodeFoldingTree::collapseToplevelNodes()
{
  qDebug()<<QString("collapse top level nodes");
}

void AbstractKateCodeFoldingTree::decrementBy1(QVector<KateCodeFoldingNodeTemp *> &nodesLine)
{
  foreach (KateCodeFoldingNodeTemp *node, nodesLine) {
    node->setLine(node->getLine() - 1);
  }
}

/*
void AbstractKateCodeFoldingTree::deleteNode(int pos)
{
  // step 0 - find the node that will be deleted
  if (pos >= m_lineMapping.size())
    return;
  if (pos <= 0)
    return;
  KateCodeFoldingNodeTemp *deletedNode = findNodeAt(pos);
  if (deletedNode->type > 0) {
    deleteStartNode(pos);
  }
  else {
    deleteEndNode(pos);
  }
}
*/

void AbstractKateCodeFoldingTree::deleteNodeFromMap(KateCodeFoldingNodeTemp *node)
{
  QVector<KateCodeFoldingNodeTemp *> mapping = m_lineMapping[node->getLine()];
  int index;
  for (index = 0 ; index < mapping.size() ; index ++) {
    if (mapping[index]->getColumn() == node->getColumn()) {
      mapping.remove(index);
      break;
    }
  }
  m_lineMapping[node->getLine()] = mapping;
  //delete node; // !!!
}

void AbstractKateCodeFoldingTree::deleteEndNode(KateCodeFoldingNodeTemp* deletedNode)
{
  deleteNodeFromMap(deletedNode);

  // step 1 - this node is removed from the tree
  deletedNode->parentNode->removeChild(deletedNode);

  // step 2 - recalibrate folding tree starting from parent
  if (deletedNode->parentNode->type)
    deletedNode->parentNode->updateSelf();

  // step 3 - node is deleted
  delete deletedNode;
}

void AbstractKateCodeFoldingTree::deleteStartNode(KateCodeFoldingNodeTemp* deletedNode)
{
  deleteNodeFromMap(deletedNode);

  // step 1 - all its startChildren are inherited by its parent
  //deletedNode->parentNode->mergeChildren(deletedNode->parentNode->m_startChildren,deletedNode->m_startChildren);
  KateCodeFoldingNodeTemp *heir = fineNodeAbove(deletedNode->position);
  heir->mergeChildren(heir->m_startChildren,deletedNode->m_startChildren);

  // step 2 - this node is removed from the tree
  deletedNode->parentNode->removeChild(deletedNode);

  // step 3 - parent inherits shortage and endChildren too
  heir->updateParent(deletedNode->m_endChildren,deletedNode->shortage - 1);

  // step 4 - node is deleted
  delete deletedNode;
}

void AbstractKateCodeFoldingTree::ensureVisible(int l)
{
  qDebug()<<QString("ensure visible : %1").arg(l);
}

void AbstractKateCodeFoldingTree::expandOne(int realLine, int numLines)
{
  qDebug()<<QString("expand one : %1 ; %2").arg(realLine).arg(numLines);
}

void AbstractKateCodeFoldingTree::expandToplevelNodes(int numLines)
{
  qDebug()<<QString("expand top level nodes at %1").arg(numLines);
}

// Searches for the first start node above
KateCodeFoldingNodeTemp* AbstractKateCodeFoldingTree::fineNodeAbove(KateDocumentPosition startingPos)
{
  for (int line = startingPos.line ; line >= 0 ; line --) {
    QVector <KateCodeFoldingNodeTemp*> tempMap = m_lineMapping[line];
    for (int column = tempMap.size() - 1 ; column >= 0 ; column --) {
      // The search for a "start node"
      // We still have to check positions becose the parent might be on the same line
      if (tempMap[column]->type > 0 && tempMap[column]->position < startingPos)
        return tempMap[column];
    }
  }

  // The root node is the default parent
  return m_root;
}

// Searches for the node placed on a specific position
KateCodeFoldingNodeTemp* AbstractKateCodeFoldingTree::findNodeAt(KateDocumentPosition position)
{
  QVector <KateCodeFoldingNodeTemp*> tempVector = m_lineMapping[position.line];

  foreach (KateCodeFoldingNodeTemp *node,tempVector) {
    if (node->position == position)
      return node;
  }
  return NULL;
}

KateCodeFoldingNodeTemp* AbstractKateCodeFoldingTree::findNodeForPosition(unsigned int l, unsigned int c)
{
  return findNodeAt(KateDocumentPosition(l,c));
}


// Search for the parent of the new node that will be created at position startingPos
KateCodeFoldingNodeTemp* AbstractKateCodeFoldingTree::findParent(KateDocumentPosition startingPos,int childType)
{
  for (int line = startingPos.line ; line >= 0 ; line --) {
    QVector <KateCodeFoldingNodeTemp*> tempLineMap = m_lineMapping[line];
    for (int i = tempLineMap.size() - 1 ; i >= 0 ; i --) {

      // The parent must be a "start node"
      if (tempLineMap[i]->type < 0)
        continue;

      // The parent must be on a lower position that its child
      if (tempLineMap[i]->getPosition() > startingPos)
        continue;

      // If it's inserted an "end node", then there are no other conditions
      if (childType < 0)
        return tempLineMap[i];

      // If the node has a shortage, we found the parent node
      if (tempLineMap[i]->hasMatch() == false)
        return tempLineMap[i];

      // If this node has a match and its matching node's
      // position is lower then current node, we found the parent
      if (tempLineMap[i]->m_endChildren.last()->position > startingPos)
        return tempLineMap[i];
    }
  }

  // The root node is the default parent
  return m_root;
}

void AbstractKateCodeFoldingTree::fixRoot(int endLine)
{
  m_rootMatch->setLine(endLine);
}

int AbstractKateCodeFoldingTree::getLineDepth(int line)
{
  // If line is invalid, then it is possible to decrease the depth by 1
  int delta = 0;
  while (line >=0 && (m_lineMapping.contains(line) == false || m_lineMapping[line].empty())) {
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

int AbstractKateCodeFoldingTree::getLineDepth(int line, bool &validEndings)
{
  validEndings = false;

  // If there is a start node on this line,
  // then the depth of this line is the depth of the last start node (biggest depth)
  KateCodeFoldingNodeTemp *lastNode = m_lineMapping[line].last();
  if (lastNode->type > 0) {
    return lastNode->getDepth();
  }

  // If we have an end node on this line,
  // then the depth of this line is the depth of the last VALID end node on this line (smallest depth)
  if (m_lineMapping[line].first()->type < 0) {
    QVector<KateCodeFoldingNodeTemp *> tempLineMap = m_lineMapping[line];
    int index;
    for (index = 0 ; index < tempLineMap.size() ; ++ index) {
      KateCodeFoldingNodeTemp* startMatch = tempLineMap[index]->parentNode->getStartMatching(tempLineMap[index]);
      if (tempLineMap[index]->type > 0 || startMatch->type == 0)
        break;
    }
    if (index > 0) {
      validEndings = true;
      KateCodeFoldingNodeTemp* startMatch = tempLineMap[index - 1]->parentNode->getStartMatching(tempLineMap[index - 1]);
      return startMatch->getDepth();
    }
    /*KateCodeFoldingNodeTemp* firstNode = m_lineMapping[line].first();
    KateCodeFoldingNodeTemp* startMatch = firstNode->parentNode->getStartMatching(firstNode);
    if (startMatch->type) {
      return startMatch->getDepth();
    }*/

    // If there are only invalid end nodes, then the depts is "0"
    return 0;
  }
}

void AbstractKateCodeFoldingTree::getLineInfo(KateLineInfo *info, unsigned int line)
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
  if (m_lineMapping.contains(line) && m_lineMapping[line].isEmpty() == false) {
   QVector<KateCodeFoldingNodeTemp *> tempLineMap = m_lineMapping[line];
   foreach(KateCodeFoldingNodeTemp *node, tempLineMap) {
     // If our node it's a start node
     if (node->type > 0) {
       // Check if it is visibile
       if (node->visible) {
         info->startsVisibleBlock = true;
       }
       else {
         info->startsInVisibleBlock = true;
       }
     }

     // If our node it's a end node
     else {
       // check if it's matching node is the rood node (is invalid)
       if (node->parentNode->getStartMatching(node)->type) {
         info->endsBlock = true;
       }
       else {
         info->invalidBlockEnd = true;
       }
     }
   }
  }
}

void AbstractKateCodeFoldingTree::incrementBy1(QVector<KateCodeFoldingNodeTemp *> &nodesLine)
{
  foreach (KateCodeFoldingNodeTemp *node, nodesLine) {
    node->setLine(node->getLine() + 1);
  }
}

void AbstractKateCodeFoldingTree::insertNodeIntoMap(KateCodeFoldingNodeTemp* newNode)
{
  // If this line is not empty
  if (m_lineMapping.contains(newNode->getLine())) {
    QVector<KateCodeFoldingNodeTemp *> tempLineMap = m_lineMapping[newNode->getLine()];
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
    QVector<KateCodeFoldingNodeTemp *> tempLineMap;
    tempLineMap.append(newNode);
    m_lineMapping[newNode->getLine()] = tempLineMap;
  }
}

void AbstractKateCodeFoldingTree::insertEndNode(int type, KateDocumentPosition pos)
{
  // step 0 - set newNode's parameters
  // find the parent of the new node
  KateCodeFoldingNodeTemp *parentNode = findParent(pos,type);
  KateCodeFoldingNodeTemp *newNode = new KateCodeFoldingNodeTemp(parentNode,type,pos);

  // step1 - add the new node to the map and to its parent node
  insertNodeIntoMap(newNode);
  parentNode->addChild(newNode);

  // step 1 - call updateSelf() for parent node to recalibrate tree
  // (only if node's parent is not the root node)
  if (parentNode->type)
    parentNode->updateSelf();
}

void AbstractKateCodeFoldingTree::insertStartNode(int type, KateDocumentPosition pos)
{
  // step 0 - set newNode's parameters
  // find the parent of the new node
  KateCodeFoldingNodeTemp *parentNode = findParent(pos,type);
  KateCodeFoldingNodeTemp *newNode = new KateCodeFoldingNodeTemp(parentNode,type,pos);

  // step 1 - devide parent's startChildrenList
  QVector <KateCodeFoldingNodeTemp*> tempList(parentNode->m_startChildren);
  sublist(parentNode->m_startChildren,tempList,KateDocumentPosition(0,0),newNode->position);
  sublist(newNode->m_startChildren,tempList,newNode->position,INFposition);

  // step 2 - devide parent's endChildrenList (or inherit shortage)
  if (parentNode->shortage > 0 && parentNode->type) {
    newNode->shortage = parentNode->shortage;
  }
  else {
    tempList = parentNode->m_endChildren;
    sublist(newNode->m_endChildren,tempList,newNode->position,INFposition);
    foreach (KateCodeFoldingNodeTemp *child, newNode->m_endChildren) {
      parentNode->remove(child,parentNode->m_endChildren);
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

void AbstractKateCodeFoldingTree::lineHasBeenInserted(int line)
{
  QMap <int, QVector <KateCodeFoldingNodeTemp*> > tempMap = m_lineMapping;
  QMapIterator <int, QVector <KateCodeFoldingNodeTemp*> > iterator(tempMap);
  QVector <KateCodeFoldingNodeTemp*> tempVector;
  m_lineMapping.clear();

  // Coppy the lines before "line"
  while (iterator.hasNext() && iterator.peekNext().key() < line) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    m_lineMapping.insert(key,tempVector);
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

void AbstractKateCodeFoldingTree::lineHasBeenRemoved(int line)
{
  QMap <int, QVector <KateCodeFoldingNodeTemp*> > tempMap = m_lineMapping;
  QMapIterator <int, QVector <KateCodeFoldingNodeTemp*> > iterator(tempMap);
  QVector <KateCodeFoldingNodeTemp*> tempVector;
  m_lineMapping.clear();

  // Coppy the lines before "line"
  while (iterator.hasNext() && iterator.peekNext().key() < line) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    m_lineMapping.insert(key,tempVector);
    iterator.next();
  }

  //
  while (iterator.hasNext() && iterator.peekNext().key() == line) {
    // Coppy the old line
    tempVector = iterator.peekNext().value();
    m_lineMapping.insert(line,tempVector);

    // And remove all the nodes from it
    while (tempVector.isEmpty() == false) {
      deleteNode(tempVector.last());
      tempVector = m_lineMapping[line];
    }
    iterator.next();
  }

  // All the other lines has to be updated before moved
  while (iterator.hasNext()) {
    int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    decrementBy1(tempVector);
    m_lineMapping.insert(key - 1,tempVector);
    iterator.next();
  }

  // If the deleted line was replaced with an empty line, the map entry will be deleted
  if (m_lineMapping[line].empty()) {
    m_lineMapping.remove(line);
  }
}

// Update columns when "colsChanged" flag from updateLine() is "true"
// newColumns[2 * k + 1] = position of node k
void AbstractKateCodeFoldingTree::setColumns(int line, QVector<int> newColumns)
{
  QVector<KateCodeFoldingNodeTemp*> oldLineMapping = m_lineMapping[line];

  int index = 1;
  foreach (KateCodeFoldingNodeTemp* tempNode, oldLineMapping) {
    tempNode->setColumn(newColumns[index]);
    index += 2;
  }
}

// puts in dest Vector a sublist of source Vector
void AbstractKateCodeFoldingTree::sublist(QVector<KateCodeFoldingNodeTemp *> &dest, QVector<KateCodeFoldingNodeTemp *> source,
                                          KateDocumentPosition begin, KateDocumentPosition end)
{
  dest.clear();
  foreach (KateCodeFoldingNodeTemp *node, source) {
    if (node->position >= end && end != INFposition)
      break;
    if (node->position >= begin) {
      dest.append(node);
    }
  }
}

void AbstractKateCodeFoldingTree::toggleRegionVisibility(int l) {
  qDebug()<<QString("toogle ... at %1").arg(l);
}

// changed = true, if there is there is a new node on the line / a node was deleted from the line
// colschanged = true only if node's column changes (nodes de not appear/disappear)
void AbstractKateCodeFoldingTree::updateLine(unsigned int line, QVector<int> *regionChanges, bool *updated, bool changed, bool colsChanged)
{
  if (!changed && !colsChanged) {
    *updated = false;
    return;
  }

  if (colsChanged) {
    setColumns(line, *regionChanges);
    *updated = true;
    return;
  }

  // changed == true
  updateMapping(line, *regionChanges);
  *updated = true;

  buildTreeString(m_root,1);
  qDebug()<<treeString;
  buildStackString();
  //qDebug()<<stackString;
  QMessageBox alert;
  alert.setText("ERROR");
  if (!isCorrect()) {
    alert.exec();
  }
}

// Update mapping when "changhed" flag from updateLine() is "true" - nodes inserted or deleted
// newColumns[2 * k + 1] = position of node k
void AbstractKateCodeFoldingTree::updateMapping(int line, QVector<int> newColumns)
{
  QVector<KateCodeFoldingNodeTemp*> oldLineMapping = m_lineMapping[line];
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
        insertNode(nodeType, KateDocumentPosition(line,nodeColumn - 1));
      }
      else {
        insertNode(nodeType, KateDocumentPosition(line,nodeColumn));
      }
    }

    // If the nodes compared have the same type,
    // then we update the column (maybe the column has changed).
    else if (oldLineMapping[index_old]->type == newColumns[index_new - 1]) {
      if (newColumns[index_new - 1] < 0) {
        oldLineMapping[index_old]->setColumn(newColumns[index_new] - 1);
      }
      else  {
        oldLineMapping[index_old]->setColumn(newColumns[index_new]);
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


// Debug methods

void AbstractKateCodeFoldingTree::printMapping() {
  qDebug()<<"\n***************** New Line mapping print ******************\n";
  QMapIterator <int, QVector <KateCodeFoldingNodeTemp*> > iterator(m_lineMapping);
  while (iterator.hasNext()) {
    QVector <KateCodeFoldingNodeTemp*> tempVector = iterator.peekNext().value();
    int key = iterator.next().key();
    qDebug()<<(QString("\nLine %1").arg(key));
    foreach (KateCodeFoldingNodeTemp *node, tempVector) {
      qDebug()<<(QString("(%1,%2)").arg(node->type).arg(node->getColumn()));
    }
    qDebug()<<(QString("\nEnd of Line %1....").arg(key));
  }
  qDebug()<<"\n**********************************************************\n";
}

void AbstractKateCodeFoldingTree::buildStackString()
{
  QStack<KateCodeFoldingNodeTemp *> testStack;
  stackString.clear();
  testStack.clear();
  int level = 0;
  int index = -1;
  int nPops = 0;

  QVector <KateCodeFoldingNodeTemp*> tempVector;
  QMapIterator <int, QVector <KateCodeFoldingNodeTemp*> > iterator(m_lineMapping);
  iterator.next();
  testStack.push(m_root);
  while (iterator.hasNext()) {
    //int key = iterator.peekNext().key();
    tempVector = iterator.peekNext().value();
    iterator.next();
    foreach (KateCodeFoldingNodeTemp *node, tempVector) {
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
        stackString.append(QString("{ (l=%1, c=%2, pL=%3, pC=%4)").arg(node->getLine()).
                           arg(node->getColumn()).arg(testStack.top()->getLine()).arg(testStack.top()->getColumn()));
        testStack.push(node);
      }
      else if (node->type < 0) {
        stackString.append("\n");
        for (int i = 0 ; i < level ; ++ i)
          stackString.append(QString("   "));
        stackString.append(QString("} (l=%1, c=%2, pL=%3, pC=%4)").arg(node->getLine()).
                           arg(node->getColumn()).arg(testStack.top()->getLine()).arg(testStack.top()->getColumn()));
        nPops ++;
      }
    }
  }
}

void AbstractKateCodeFoldingTree::buildTreeString(KateCodeFoldingNodeTemp *node, int level)
{
  if (node->type == 0)
    treeString.clear();
  else
    treeString.append("\n");
  for (int i = 0 ; i < level-1 ; ++ i)
    treeString.append(QString("   "));

  if (node->type > 0)
  {
    treeString.append(QString("{ (l=%1, c=%2, pL=%3, pC=%4)").arg(node->getLine()).
                            arg(node->getColumn()).arg(node->parentNode->getLine()).arg(node->parentNode->getColumn()));
  }
  else if (node->type < 0)
  {
    treeString.append(QString("} (l=%1, c=%2, pL=%3, pC=%4)").arg(node->getLine()).
                            arg(node->getColumn()).arg(node->parentNode->getLine()).arg(node->parentNode->getColumn()));
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

bool AbstractKateCodeFoldingTree::isCorrect()
{
  return  (stackString.compare(treeString) == 0 ? true : false);
}
