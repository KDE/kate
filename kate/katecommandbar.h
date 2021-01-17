#include <QMenu>
#include <QList>

class QTreeView;
class QLineEdit;
class CommandModel;
class QAction;
class CommandBarFilterModel;

class KateCommandBar : public QMenu
{
    Q_OBJECT
public:
    KateCommandBar(QWidget* parent = nullptr);

    void updateBar(QList<QAction*> actions);

    void updateViewGeometry();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void slotReturnPressed();
    void reselectFirst();

private:
    QTreeView* m_treeView;
    QLineEdit* m_lineEdit;
    CommandModel* m_model;
    CommandBarFilterModel* m_proxyModel;
};
