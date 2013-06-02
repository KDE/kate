
#ifndef TARGETS_H
#define TARGETS_H

#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QTreeWidget>
#include <QtGui/QWidget>
#include "klineedit.h"
#include "kpushbutton.h"
#include <kcombobox.h>

class TargetsUi: public QWidget
{
    Q_OBJECT

public:
    TargetsUi(QWidget *parent = 0);

    QComboBox   *targetCombo;
    QToolButton *newTarget;
    QToolButton *copyTarget;
    QToolButton *deleteTarget;
    QFrame      *line;
    
    QLabel      *dirLabel;
    KLineEdit   *buildDir;
    QToolButton *browse;
    QLabel      *quickLabel;
    KLineEdit   *quickCmd;
    QTreeWidget *targetsList;

    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *deleteButton;
    QPushButton *buildButton;
    QPushButton *defButton;
    QPushButton *cleanButton;

protected:
    void resizeEvent(QResizeEvent *event);

private Q_SLOTS:
    void editTarget(const QString &text);
    
private:
    void setBottomLayout();
    void setSideLayout();

    int  m_widgetsHeight;
    bool m_useBottomLayout;
};

#endif
