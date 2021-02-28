#ifndef QUICKDIALOG_H
#define QUICKDIALOG_H

#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QTreeView>

class QAbstractItemModel;
class QTreeView;
class QLineEdit;
namespace KTextEditor
{
class MainWindow;
}

class QuickDialog : public QMenu
{
    Q_OBJECT
public:
    QuickDialog(QWidget *mainWindow /* QAbstractItemModel *model*/ /*, QuickDialogProxyModel *proxyModel*/);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void updateViewGeometry();
    void clearLineEdit();

protected Q_SLOTS:
    virtual void slotReturnPressed() = 0;

protected:
    QTreeView m_treeView;
    QLineEdit m_lineEdit;

private:
    QPointer<QWidget> m_mainWindow;
};

#endif // QUICKDIALOG_H
