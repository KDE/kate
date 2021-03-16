#ifndef KATEPROJECTTODOVIEW_H
#define KATEPROJECTTODOVIEW_H

#include <QWidget>
#include <QPointer>

namespace KTextEditor
{
class MainWindow;
}

class KateProjectTODOView : public QWidget
{
    Q_OBJECT
public:
    explicit KateProjectTODOView(KTextEditor::MainWindow *mainWin, const QString &projectPath);
    void refresh();

private:
    void reload(const QByteArray &data);

    QString m_projectPath;
    class QTreeView *m_treeView;
    class QStandardItemModel *m_model;
    QPointer<KTextEditor::MainWindow> m_mainWindow;
};

#endif // KATEPROJECTTODOVIEW_H
