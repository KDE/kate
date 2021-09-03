#include <QDialog>
#include <QLabel>
#include <QListView>
#include <QStandardItemModel>

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
    QListView m_listView;
};
