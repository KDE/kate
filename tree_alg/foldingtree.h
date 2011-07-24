#ifndef FOLDINGTREE_H
#define FOLDINGTREE_H

#include "QVector"
#include "QString"
#include "QStack"

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

    FoldingNode *matchingNode();
    inline bool hasMatch() { return (m_endChildren.size() > 0); }

    void addChild(FoldingNode *node);
    FoldingNode* removeChild(FoldingNode *node);
    QVector<FoldingNode *> removeChilrenInheritedFrom(FoldingNode *node);
    void add(FoldingNode *node, QVector<FoldingNode*> &m_childred);
    bool remove(FoldingNode *node, QVector<FoldingNode*> &m_childred);
    void setParent();

    void mergeChildren(QVector <FoldingNode*> &list1, QVector <FoldingNode*> &list2);
    void updateParent(QVector <FoldingNode *> newExcess, int newShortage);
    void updateSelf();
    bool isDuplicated(FoldingNode *node);
    bool hasBrothers();

    // debug methods
    bool contains(FoldingNode *node);
    bool isDuplicatedPrint(FoldingNode *node);
    QString toString(int level = 1);

};

class FoldingTree
{
public:
    FoldingTree();
    ~FoldingTree();

    FoldingNode *root;
    QVector <FoldingNode *> nodeMap;

    // debug info
    static int newNodePos;
    static QVector<QString> history;
    static QVector<QString> movesHistory;
    static int nOps;
    // debug info

    static bool displayDetails;
    static bool displayChildrenDetails;
    static bool displayDuplicates;

    void insertStartNode (int pos);
    void insertEndNode (int pos);
    void deleteStartNode (int pos);
    void deleteEndNode (int pos);
    void deleteNode (int pos);

    void increasePosition (int startingPos);
    void decreasePosition (int startingPos);
    FoldingNode* findParent (int startingPos, int childType);
    FoldingNode* fineNodeAbove (int startingPos);

    void sublist(QVector<FoldingNode*> &dest, QVector<FoldingNode*> source, int begin, int end);
    FoldingNode* findNodeAt(int position);

    QString toString();

    // Automatic Testing
    QString treeString;
    QString stackString;
    void buildStackString();                                // Will build the output using the stack alg
    void buildTreeString(FoldingNode *node, int level);     // Will build the output using the tree alg
    bool isCorrect();                                       // will compare the stackString with the treeString

};

#endif // FOLDINGTREE_H
