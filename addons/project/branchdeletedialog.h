#include <QDialog>
#include <QLabel>
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
    QStandardItemModel m_model;
    QTreeView m_listView;
};
