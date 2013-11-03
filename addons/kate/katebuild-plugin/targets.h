
#ifndef TARGETS_H
#define TARGETS_H

#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>
#include "klineedit.h"
#include "kpushbutton.h"
#include <kcombobox.h>


#define COL_DEFAULT_TARGET 0
#define COL_CLEAN_TARGET 1
#define COL_NAME 2
#define COL_COMMAND 3


class TargetsUi: public QWidget
{
    Q_OBJECT

public:
    TargetsUi(QWidget *parent = 0);

    QLabel      *targetLabel;
    QComboBox   *targetCombo;
    QToolButton *newTarget;
    QToolButton *copyTarget;
    QToolButton *deleteTarget;

    QLabel      *dirLabel;
    KLineEdit   *buildDir;
    QToolButton *browse;
    QTableWidget *targetsList;

    QToolButton *addButton;
    QToolButton *deleteButton;
    QToolButton *buildButton;

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
