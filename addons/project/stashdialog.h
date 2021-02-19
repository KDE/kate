/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QMenu>

class QTreeView;
class QLineEdit;
class BranchesDialogModel;
class QAction;
class StashFilterModel;
class KActionCollection;
class QStandardItemModel;
class QProcess;
class GitWidget;

namespace KTextEditor
{
class MainWindow;
}

namespace GitUtils
{
struct CheckoutResult;
}

class StashDialog : public QMenu
{
    Q_OBJECT
public:
    enum Mode {
        None,
        Stash,
        StashKeepIndex,
        StashUntrackIncluded,
        StashPopLast,
        StashPop,
        StashDrop,
        StashApply,
        StashApplyLast,
        ShowStashContent,
    };

    StashDialog(QWidget *parent, KTextEditor::MainWindow *mainWindow);

    void openDialog(Mode mode);

    void updateViewGeometry();

    Q_SIGNAL void branchChanged(const QString &branch);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void slotReturnPressed();
    void reselectFirst();
    void sendMessage(const QString &message, bool warn);

private:
    void stash(bool keepIndex, bool includeUntracked);
    void getStashList();
    void popStash(const QByteArray &index, const QString &command = QStringLiteral("pop"));
    void applyStash(const QByteArray &index);
    void dropStash(const QByteArray &index);
    void showStash(const QByteArray &index);

    QTreeView *m_treeView;
    QLineEdit *m_lineEdit;
    QStandardItemModel *m_model;
    StashFilterModel *m_proxyModel;
    KTextEditor::MainWindow *m_mainWindow;
    GitWidget *m_gitwidget;
    QString m_projectPath;
    Mode m_currentMode = None;
};
