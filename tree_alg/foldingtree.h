#ifndef FOLDINGTREE_H
#define FOLDINGTREE_H

#include "QVector"
#include "QString"

class FoldingNode
{
  public:
    FoldingNode(int pos, int typ, FoldingNode *par);
    ~FoldingNode();

    int position;
    int type;
    FoldingNode *parent;
    int shortage;

    QVector<FoldingNode*> m_startChildren;  // Node's startChildren list
    QVector<FoldingNode*> m_endChildren;    // Node's endChildren list

    FoldingNode* matchingNode();

    void addChild(FoldingNode *node);
    void removeChild(FoldingNode *node);
    void add(FoldingNode *node, QVector<FoldingNode*> &m_childred);
    void remove(FoldingNode *node, QVector<FoldingNode*> &m_childred);
    void setParent();

    void mergeChildren(QVector <FoldingNode*> &list1, QVector <FoldingNode*> &list2);
    void updateParent(QVector <FoldingNode *> newExcess, int newShortage);
    void updateSelf();

    QString toString(int level = 0);

};

class FoldingTree
{
public:
    FoldingTree();
    ~FoldingTree();

    FoldingNode *root;
    QVector <FoldingNode *> nodeMap;

    void insertStartNode (int pos);
    void insertEndNode (int pos);
    void deleteStartNode (int pos);
    void deleteEndNode (int pos);
    void deleteNode (int pos);

    void increasePosition (int startingPos);
    void decreasePosition (int startingPos);
    FoldingNode* findParent (int startingPos);

    void sublist(QVector<FoldingNode*> &dest, QVector<FoldingNode*> source, int begin, int end);
    FoldingNode* findNodeAt(int position);

    QString toString();

};

#endif // FOLDINGTREE_H
