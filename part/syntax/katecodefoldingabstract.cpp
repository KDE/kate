#include "katecodefoldingabstract.h"
#include "ktexteditor/cursor.h"

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp() :
    parentNode(0),
    line(0),
    column(0),
    startLineValid(false),
    type(0),
    visible(true)
{
}

KateCodeFoldingNodeTemp::KateCodeFoldingNodeTemp(KateCodeFoldingNodeTemp *parent, signed char typ, unsigned int l):
    parentNode(parent),
    line(l),
    column(0),
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

  begin->setLine(line);
  begin->setColumn(column);

  return true;
}

// The tree it's not used. It is inherited from the previuos implementation
// and kept for compatibility (at least for now...)
bool KateCodeFoldingNodeTemp::getEnd(AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor *end) {
  if ( ! haveMatch() ) return false;

  end->setLine(matchingNode()->line);
  end->setColumn(matchingNode()->column);

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

// changed = true, if there is there is a new node on the line / a node was deleted from the line
// colschanged = true only if node's column changes (nodes de not appear/disappear)
void AbstractKateCodeFoldingTree::updateLine(unsigned int line, QVector<int> *regionChanges, bool *updated, bool changed, bool colschanged)
{
  if (!changed && !colschanged)
    return;

  if (colschanged) {
    setColumns(line, *regionChanges);
    return;
  }

  // changed == true
  updateMapping(line, *regionChanges);

}

void AbstractKateCodeFoldingTree::setColumns(int line, QVector<int> newColumns)
{
  QVector<KateCodeFoldingNodeTemp*> oldMapping = lineMapping[line];
  int index = 0;

  foreach (KateCodeFoldingNodeTemp* tempNode, oldMapping) {
    tempNode->setColumn(newColumns[index]);
    index += 2;
  }
}

void AbstractKateCodeFoldingTree::updateMapping(int line, QVector<int> newColumns)
{
  QVector<KateCodeFoldingNodeTemp*> oldMapping = lineMapping[line];
  QVector<KateCodeFoldingNodeTemp*> newMapping;
  int index_old = 0;
  int index_new = 0;

  for ( ; index_new < newMapping.size() ; ++index_new, index_old += 2 ) {
    if (index_old >= oldMapping.size()) {                       // New node(s) was / were inserted
      ;// newMapping->insert(new Node(newColumns[index_new])    // insert new node ...
    }
    else if (oldMapping[index_old] == newMapping[index_new]) {  // Same nodes (columns were modified)
      ;// newMapping->coppy(oldMapping[index_old],newColumns[index_new]->columns)
    }
    else {                                                      // Nodes are different. Assume a new node was inserted
      ;// newMapping->insert(new Node(newColumns[index_new])    // insert new node ...
    }
  }

  for ( ; index_old < oldMapping.size() ; index_old += 2) {     // No of iteretations here = no of nodes deleted
    ; // deleteNode()
  }

  oldMapping.clear();
  lineMapping.insert(line,newMapping);
}
