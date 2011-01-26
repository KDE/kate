
#ifndef TARGETS_H
#define TARGETS_H

#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "klineedit.h"
#include "kpushbutton.h"

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
    QLabel      *buildLabel;
    QComboBox   *buildCmds;
    QLabel      *cleanLabel;
    QComboBox   *cleanCmds;
    QLabel      *quickLabel;
    QComboBox   *quickCmds;

protected:
    void resizeEvent(QResizeEvent *event);

private Q_SLOTS:
    void addBuildCmd();
    void addCleanCmd();
    void addQuickCmd();

    void delBuildCmd();
    void delCleanCmd();
    void delQuickCmd();
    
    void editTarget(const QString &text);
    void editBuildCmd(const QString &text);
    void editCleanCmd(const QString &text);
    void editQuickCmd(const QString &text);
    
private:
    void setBottomLayout();
    void setSideLayout();

    int  m_widgetsHeight;
    bool m_useBottomLayout;

    QToolButton  *m_addBuildCmd;
    QToolButton  *m_delBuildCmd;

    QToolButton  *m_addCleanCmd;
    QToolButton  *m_delCleanCmd;

    QToolButton  *m_addQuickCmd;
    QToolButton  *m_delQuickCmd;
};

#endif
