#include <QDialog>
#include <QStandardItemModel>
#include <QTreeView>

class BranchDeleteDialog : public QDialog
{
    Q_OBJECT
public:
    BranchDeleteDialog(const QString &dotGitPath, QWidget *parent = nullptr);
    QStringList branchesToDelete() const;

private:
    void loadBranches(const QString &dotGitPath);
    void updateLabel(QStandardItem *item);
    void onCheckAllClicked(bool);
    QStandardItemModel m_model;
    QTreeView m_listView;
};
