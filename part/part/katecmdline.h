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

  private:
    KateView *m_view;
};

#endif
