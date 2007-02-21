#ifndef _KATE_TOOLTIP_MENU_
#define _KATE_TOOLTIP_MENU_
#include <QMenu>

class QLabel;

class KateToolTipMenu: public QMenu
{
    Q_OBJECT
  public:
    KateToolTipMenu(QWidget *parent = 0);
    virtual ~KateToolTipMenu();
  protected:
    virtual bool event(QEvent*);
  private:
    QAction *m_currentAction;
    QLabel *m_toolTip;
  private Q_SLOTS:
    void slotHovered(QAction*);
};


#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

