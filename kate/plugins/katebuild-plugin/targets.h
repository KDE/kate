
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
public:
    TargetsUi(QWidget *parent = 0);

    QComboBox *targetCombo;
    KPushButton *newTarget;
    KPushButton *copyTarget;
    KPushButton *deleteTarget;
    QFrame *line;
    
    QLabel *dirLabel;
    KLineEdit *buildDir;
    QToolButton *browse;
    QLabel *configLabel;
    KLineEdit *configCmd;
    QLabel *buildLabel;
    KLineEdit *buildCmd;
    QLabel *cleanLabel;
    KLineEdit *cleanCmd;
    QLabel *quickLabel;
    KLineEdit *quickCmd;

protected:
    void resizeEvent(QResizeEvent *event);

private:
    void setBottomLayout();
    void setSideLayout();
    int  m_widgetsHeight;
    bool m_useBottomLayout;
};

#endif
