#include "katecodefoldingabstract.h"
#include "ktexteditor/cursor.h"

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp() :
    parentNode(0),
    position(0,0),
    startLineValid(false),
    type(0),
    visible(true)
{
}

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp(KateCodeFoldingNodeTemp *parent, signed char typ, KateDocumentPosition &pos):
    parentNode(parent),
    position(pos),
    startLineValid(true),
    type(typ),
    visible(true)
{
}

KateCodeFoldingNodeTemp::~KateCodeFoldingNodeTemp()
{
  clearStartChildren ();
  clearEndChildren ();
}

// The tree it's not used. It is inherited from the previuos implementation
// and kept for compatibility (at least for now...)
bool KateCodeFoldingNodeTemp::getBegin(AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor* begin) {
  if (!startLineValid) return false;

  begin->setLine(position.line);
  begin->setColumn(position.column);

  return true;
}

// The tree it's not used. It is inherited from the previuos implementation
// and kept for compatibility (at least for now...)
bool KateCodeFoldingNodeTemp::getEnd(AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor *end) {
  if ( ! haveMatch() ) return false;

  end->setLine(matchingNode()->position.line);
  end->setColumn(matchingNode()->position.column + 1);
  // We want to fold "}" too (+ 1)

  return true;
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

void KateCodeFoldingNodeTemp::clearStartChildren()
{
  for (int i = 0; i < m_startChildren.size(); ++i)
    delete m_startChildren[i];

  m_startChildren.clear();;
}

void KateCodeFoldingNodeTemp::clearEndChildren()
{
  for (int i = 0; i < m_endChildren.size(); ++i)
    delete m_endChildren[i];

  m_endChildren.clear();;
}
// End of KateCodeFoldingNodeTemp Class

//-------------------------------------------------------//

// Start of AbstractKateCodeFoldingTree
AbstractKateCodeFoldingTree::AbstractKateCodeFoldingTree(KateBuffer* buffer) :
    m_buffer(buffer)
{}

AbstractKateCodeFoldingTree::~AbstractKateCodeFoldingTree()
{}

void AbstractKateCodeFoldingTree::insertNode(int nodeType, KateDocumentPosition pos)
{
  KateCodeFoldingNodeTemp *newNode = new KateCodeFoldingNodeTemp(NULL,nodeType,pos);
  // If this line is not empty
  if (m_lineMapping.contains(pos.line)) {
    QVector<KateCodeFoldingNodeTemp *> mapping = m_lineMapping[pos.line];
    int index;
    for (index = 0 ; index < mapping.size() ; index++) {
      if (mapping[index]->getColumn() > newNode->getColumn())
        break;
    }
    mapping.insert(index,newNode);
    m_lineMapping[pos.line] = mapping;
  }
  // If line is empty
  else  {
    QVector<KateCodeFoldingNodeTemp *> mapping;
    mapping.append(newNode);
    m_lineMapping[pos.line] = mapping;
  }
}

// !!!
void AbstractKateCodeFoldingTree::deleteNode(KateCodeFoldingNodeTemp *node)
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
  //delete node;
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

  // Step over deleted line
  while (iterator.hasNext() && iterator.peekNext().key() == line) {
    //int key = iterator.key();
    //tempVector = iterator.value();
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

void AbstractKateCodeFoldingTree::incrementBy1(QVector<KateCodeFoldingNodeTemp *> &nodesLine)
{
  foreach (KateCodeFoldingNodeTemp *node, nodesLine) {
    node->setLine(node->getLine() + 1);
  }
}

void AbstractKateCodeFoldingTree::decrementBy1(QVector<KateCodeFoldingNodeTemp *> &nodesLine)
{
  foreach (KateCodeFoldingNodeTemp *node, nodesLine) {
    node->setLine(node->getLine() - 1);
  }
}

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
