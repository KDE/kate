#ifndef __KATE_CMDLINE_H__
#define __KATE_CMDLINE_H__

#include <klineedit.h>

class KateView;

class KateCmdLine : public KLineEdit
{
  Q_OBJECT

  public:
    KateCmdLine (KateView *view);
    virtual ~KateCmdLine ();

  private slots:
    void slotReturnPressed ( const QString& cmd );

  protected:
    void focusInEvent ( QFocusEvent *ev );

  private:
    KateView *m_view;
    bool m_msgMode;
    QString m_oldText;
};

#endif
