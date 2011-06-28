#ifndef FOLDINGTREE_H
#define FOLDINGTREE_H

#include "QVector"
#include "QString"

class FoldingNode
{
  public:
    FoldingNode(int no, int type, FoldingNode *par);
    ~FoldingNode();

    int nodeNo;
    int type;
    FoldingNode *parent;
    int shortage;

    QVector<FoldingNode*> m_startChildren;  // Node's start children
    QVector<FoldingNode*> m_endChildren;    // Node's end children

    FoldingNode* matchingNode();

    void addChild(FoldingNode *node);
    void deleteChild(FoldingNode *node);
    void insert(FoldingNode *node, QVector<FoldingNode*> &m_childred);
    void remove(FoldingNode *node, QVector<FoldingNode*> &m_childred);

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

    void insertStartNode(int pos);
    void insertEndNode(int pos);
    void deleteNode(int pos);
    void nodeInserted(int pos);
    void nodeDeleted(int pos);
    FoldingNode* findParent(int startingPos);

    QString toString();

};

#endif // FOLDINGTREE_H
