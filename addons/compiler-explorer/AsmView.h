#ifndef KATE_CE_ASM_VIEW_H
#define KATE_CE_ASM_VIEW_H

#include <QTreeView>

class AsmView : public QTreeView
{
    Q_OBJECT
public:
    explicit AsmView(QWidget *parent);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

Q_SIGNALS:
    void scrollToLineRequested(int line);
};

#endif // KATE_CE_ASM_VIEW_H
