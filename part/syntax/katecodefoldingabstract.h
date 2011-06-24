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

class KateCodeFoldingNodeTemp: public QObject
{
  friend class AbstractKateCodeFoldingTree;

  //data members
  private:
    KateCodeFoldingNodeTemp *parentNode;      // parent

    unsigned int        line;             // node line (absolute position)
    unsigned int        column;           // node column

    bool                startLineValid;   // if "{" exists (not used by other classes)

    signed char         type;             // 0 -> toplevel / invalid ; 5 = {} ; 4 = comment ; -5 = only "}" ; 1/-1 start/end node py style
                                          // if type > 0 : start node ; if type < 0 : end node
    bool                visible;          // folded / not folded

    QVector<KateCodeFoldingNodeTemp*> m_startChildren;  // Node's start children
    QVector<KateCodeFoldingNodeTemp*> m_endChildren;    // Node's end children

  // public methods - Node's interface
  public:
    KateCodeFoldingNodeTemp ();
    KateCodeFoldingNodeTemp (KateCodeFoldingNodeTemp *par, signed char typ, unsigned int l);

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
      { column = newColumn; }
    inline void setLine(int newLine)
      { line = newLine; }
    inline void setColumn(KateCodeFoldingNodeTemp *node)
      { setColumn(node->getColumn()); }
    inline void setLine(KateCodeFoldingNodeTemp *node)
      { setLine(node->getLine()); }

    inline int getColumn() const
      { return column; }
    inline int getLine() const
      { return line; }
    // End of setters and getters

    // Children modifiers
    inline bool noStartChildren () const
      { return m_startChildren.isEmpty(); }
    inline bool noEndChildren () const
      { return m_endChildren.isEmpty(); }
    inline bool noChildren () const
      { return noStartChildren() && noEndChildren(); }

    inline int startChildCount () const
      { return m_startChildren.size(); }
    inline int endChildCount () const
      { return m_endChildren.size(); }
    inline int startCount () const
      { return startChildCount() + endChildCount(); }

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
    inline bool haveMatch() const
      { return noEndChildren() ? false : true ; }

    inline KateCodeFoldingNodeTemp* matchingNode() const
      { return haveMatch() ? endChildAt(0) : NULL; }

    inline int comparePos(KateCodeFoldingNodeTemp *otherNode) const
    {
      return (line > otherNode->line) ? 1 :
          (line < otherNode->line) ? -1 :
          (column > otherNode->column) ? 1 :
          (column < otherNode->column) ? -1 : 0;
    }

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

    KateCodeFoldingNodeTemp                          m_root;       // Tree's root node

    KateBuffer* const                                m_buffer;

    QMap < int, QVector <KateCodeFoldingNodeTemp*> > lineMapping;  // a map (line, <Vector of Nodes> from that line)

  public:
    AbstractKateCodeFoldingTree (KateBuffer *buffer);
    ~AbstractKateCodeFoldingTree ();

    void lineHasBeenInserted (int line);                          // call order for 3 lines : 1,2,3
    void lineHasBeenRemoved  (int line);                          // call order for 3 lines : 3,2,1

  protected:
    void setColumns (int line, QVector<int> newColumns);
    void updateMapping (int line, QVector<int> newColumns);

  public Q_SLOTS:
    void updateLine (unsigned int line,QVector<int>* regionChanges, bool* updated, bool changed, bool colschanged);

};

#endif // KATECODEFOLDINGABSTRACT_H

// kate: space-indent on; indent-width 2; replace-tabs on;
