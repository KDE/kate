#ifndef KATECODEFOLDINGABSTRACT_H
#define KATECODEFOLDINGABSTRACT_H

//BEGIN INCLUDES + FORWARDS
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QMapIterator>

#include "katepartprivate_export.h"

class AbstractKateCodeFoldingTree;
namespace KTextEditor { class Cursor; }
class KateBuffer;

class QString;
//END

class KateHiddenLineBlockTemp
{
  public:
    unsigned int start;
    unsigned int length;
};

class KateLineInfoTemp
{
  public:
    bool topLevel;
    bool startsVisibleBlock;
    bool startsInVisibleBlock;
    bool endsBlock;
    bool invalidBlockEnd;
    int depth;
};

class KateDocumentPosition
{
  public:
    int line;
    int column;

    KateDocumentPosition (int l, int c) :
        line(l),
        column(c)
    {
    }
    KateDocumentPosition (const KateDocumentPosition &pos) :
        line(pos.line),
        column(pos.column)
    {
    }
    KateDocumentPosition () :
        line(0),
        column(0)
    {
    }

    inline bool operator < (const KateDocumentPosition &pos) const
    {
      return (line < pos.line) ? true :
          (line == pos.line) ?
          ((column < pos.column) ? true : false)
            : false;
    }
    inline bool operator > (const KateDocumentPosition &pos) const
    {
      return (line > pos.line) ? true :
          (line == pos.line) ?
          ((column > pos.column) ? true : false)
            : false;
    }
    inline bool operator == (const KateDocumentPosition &pos) const
    {
      return (line == pos.line && column == pos.column) ? true : false;
    }
    inline bool operator != (const KateDocumentPosition &pos) const
    {
      return (line != pos.line || column != pos.column) ? true : false;
    }
    inline bool operator >=(const KateDocumentPosition &pos) const
    {
      return ((*this) > pos || (*this) == pos);
    }
};

class KateCodeFoldingNodeTemp: public QObject
{
  friend class AbstractKateCodeFoldingTree;

  //data members
  private:
    KateCodeFoldingNodeTemp *parentNode;      // parent

    KateDocumentPosition    position;

    bool                    startLineValid;   // if "{" exists (not used by other classes)

    signed char             type;             // 0 -> toplevel / invalid ; 5 = {} ; 4 = comment ; -5 = only "}" ; 1/-1 start/end node py style
                                              // if type > 0 : start node ; if type < 0 : end node
    bool                    visible;          // folded / not folded

    int                     shortage;

    QVector<KateCodeFoldingNodeTemp*> m_startChildren;  // Node's start children
    QVector<KateCodeFoldingNodeTemp*> m_endChildren;    // Node's end children

  // public methods - Node's interface
  public:
    KateCodeFoldingNodeTemp ();
    KateCodeFoldingNodeTemp (KateCodeFoldingNodeTemp *par, signed char typ, KateDocumentPosition pos);

    ~KateCodeFoldingNodeTemp ();

    inline int nodeType ()
      { return type; }

    inline bool isVisible ()
      { return visible; }

    inline KateCodeFoldingNodeTemp *getParentNode ()
      { return parentNode; }

    bool getBegin (AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor* begin);  // ??
    bool getEnd (AbstractKateCodeFoldingTree *tree, KTextEditor::Cursor *end);      // ??

  // protected methods - used by FoldingTree
  protected:

    // Setters and getters
    inline void setColumn(int newColumn)
      { position.column = newColumn; }
    inline void setLine(int newLine)
      { position.line = newLine; }
    inline void setPosition (KateDocumentPosition newPosition)
      { position = newPosition; }

    inline void setColumn(KateCodeFoldingNodeTemp *node)
      { setColumn(node->getColumn()); }
    inline void setLine(KateCodeFoldingNodeTemp *node)
      { setLine(node->getLine()); }
    inline void setPosition(KateCodeFoldingNodeTemp *node)
      { setPosition(node->getPosition()); }

    inline int getColumn() const
      { return position.column; }
    inline int getLine() const
      { return position.line; }
    inline KateDocumentPosition getPosition() const
      { return position; }
    // End of setters and getters

    // Children modifiers
    inline bool noStartChildren () const
      { return m_startChildren.isEmpty(); }
    inline bool noEndChildren () const
      { return m_endChildren.isEmpty(); }
    inline bool noChildren () const
      { return noStartChildren() && noEndChildren(); }

    inline int startChildrenCount () const
      { return m_startChildren.size(); }
    inline int endChildrenCount () const
      { return m_endChildren.size(); }
    inline int childrenCount () const
      { return startChildrenCount() + endChildrenCount(); }

    inline KateCodeFoldingNodeTemp* startChildAt (uint index) const
      { return m_startChildren[index]; }
    inline KateCodeFoldingNodeTemp* endChildAt (uint index) const
      { return m_endChildren[index]; }

    inline int findStartChild (KateCodeFoldingNodeTemp *node, uint start = 0) const
      { return m_startChildren.indexOf(node,start); }
    inline int findEndChild (KateCodeFoldingNodeTemp *node, uint start = 0) const
      { return m_endChildren.indexOf(node,start); }
    inline int findChild (KateCodeFoldingNodeTemp *node, uint start = 0) const
    { return (node->type > 0) ? findStartChild(node,start) :
        findEndChild(node,start);
    }


    inline void insertStartChild (uint index, KateCodeFoldingNodeTemp *node)
      { m_startChildren.insert(index,node); }
    inline void insertEndChild (uint index, KateCodeFoldingNodeTemp *node)
      { m_endChildren.insert(index,node); }
    inline void insertChild (uint index, KateCodeFoldingNodeTemp *node)
    {
      if (node->type > 0)
        insertEndChild(index,node);
      else
        insertEndChild(index,node);
    }

    inline void appendStartChild (KateCodeFoldingNodeTemp *node)
      { m_startChildren.append (node); }
    inline void appendEndChild (KateCodeFoldingNodeTemp *node)
      { m_endChildren.append (node); }
    inline void appendChild (KateCodeFoldingNodeTemp *node)
    {
      if (node->type > 0)
        appendStartChild(node);
      else
        appendEndChild(node);
    }

    KateCodeFoldingNodeTemp* removeStartChild (int index);
    KateCodeFoldingNodeTemp* removeEndChild (int index);

    void clearStartChildren ();
    void clearEndChildren ();
    inline void clearAllChildren ()
    {
      clearStartChildren();
      clearEndChildren();
    }

    // equivalent with endLineValid property from the previous implementation
    inline bool hasMatch() const
      { return noEndChildren() ? false : true ; }

    inline KateCodeFoldingNodeTemp* matchingNode() const
      { return hasMatch() ? endChildAt(0) : NULL; }

    inline int comparePos(KateCodeFoldingNodeTemp &otherNode) const
    {
      return (position == otherNode.position) ? 0 :
          (position < otherNode.position) ? -1 : 1;
    }

    void add(KateCodeFoldingNodeTemp *node, QVector<KateCodeFoldingNodeTemp*> &m_childred);
    void addChild(KateCodeFoldingNodeTemp *node);
    bool contains(KateCodeFoldingNodeTemp *node);
    bool hasBrothers();
    bool isDuplicated(KateCodeFoldingNodeTemp *node);
    void mergeChildren(QVector <KateCodeFoldingNodeTemp*> &list1, QVector <KateCodeFoldingNodeTemp*> &list2);
    bool remove(KateCodeFoldingNodeTemp *node, QVector<KateCodeFoldingNodeTemp*> &m_childred);
    KateCodeFoldingNodeTemp* removeChild(KateCodeFoldingNodeTemp *node);
    QVector<KateCodeFoldingNodeTemp *> removeChildrenInheritedFrom(KateCodeFoldingNodeTemp *node);
    void setParent();
    void updateParent(QVector <KateCodeFoldingNodeTemp*> newExcess, int newShortage);
    void updateSelf();

    // Debug Methods
    void printNode (int level);
};
// End of KateCodeFoldingNodeTemp Class

//-------------------------------------------------------//

// Start of AbstractKateCodeFoldingTree
class KATEPART_TESTS_EXPORT AbstractKateCodeFoldingTree : public QObject
{
  friend class KateCodeFoldingNodeTemp;

  Q_OBJECT

  private:

  // Tree's root node
    KateCodeFoldingNodeTemp*                         m_root;

    KateBuffer* const                                m_buffer;

  // a map (line, <Vector of Nodes> from that line)
    QMap < int, QVector <KateCodeFoldingNodeTemp*> > m_lineMapping;

    KateDocumentPosition                             INFposition;

  public:
    AbstractKateCodeFoldingTree (KateBuffer *buffer);
    ~AbstractKateCodeFoldingTree ();

    void incrementBy1 (QVector <KateCodeFoldingNodeTemp*> &nodesLine);
    void decrementBy1 (QVector <KateCodeFoldingNodeTemp*> &nodesLine);

    void lineHasBeenInserted (int line);                              // call order for 3 lines : 1,2,3
    void lineHasBeenRemoved  (int line);                              // call order for 3 lines : 3,2,1

    // Debug methods and members
    void printMapping();
    QString treeString;
    QString stackString;
    void buildStackString();                                // Will build the output using the stack alg

    // call : buildTreeString(root,1);
    void buildTreeString(KateCodeFoldingNodeTemp *node, int level);     // Will build the output using the tree alg
    bool isCorrect();                                       // will compare the stackString with the treeString
    // end of debug...



  protected:
  // Insert Node methods
    inline void insertNode(int nodeType, KateDocumentPosition pos)
    {
      nodeType > 0 ? insertStartNode(nodeType,pos) : insertEndNode(nodeType,pos);
    }
    void insertStartNode(int type, KateDocumentPosition pos);
    void insertEndNode(int type, KateDocumentPosition pos);
    void insertNodeIntoMap(KateCodeFoldingNodeTemp* newNode);

  // Delete Node methods
    inline void deleteNode (KateCodeFoldingNodeTemp* deletedNode)
    {
      deletedNode->type > 0 ? deleteStartNode(deletedNode) : deleteEndNode(deletedNode);
    }
    void deleteEndNode (KateCodeFoldingNodeTemp* deletedNode);
    void deleteStartNode (KateCodeFoldingNodeTemp* deletedNode);
    void deleteNodeFromMap(KateCodeFoldingNodeTemp *node);

  // Update position methods
    void changeColumn(KateCodeFoldingNodeTemp *node, int newColumn);
    void setColumns (int line, QVector<int> newColumns);
    void updateMapping (int line, QVector<int> newColumns);

  // Tree algorithm metods
    KateCodeFoldingNodeTemp* findParent (KateDocumentPosition startingPos,int childType);
    KateCodeFoldingNodeTemp* fineNodeAbove (KateDocumentPosition startingPos);
    void sublist(QVector<KateCodeFoldingNodeTemp *> &dest, QVector<KateCodeFoldingNodeTemp *> source,
                                              KateDocumentPosition begin, KateDocumentPosition end);
    KateCodeFoldingNodeTemp* findNodeAt(KateDocumentPosition position);

  public Q_SLOTS:
    void updateLine (unsigned int line,QVector<int>* regionChanges, bool* updated, bool changed, bool colschanged);

};

#endif // KATECODEFOLDINGABSTRACT_H

// kate: space-indent on; indent-width 2; replace-tabs on;
