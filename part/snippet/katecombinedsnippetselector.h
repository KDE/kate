#ifndef _KATE_COMBINED_SNIPPETSELECTOR_
#define _KATE_COMBINED_SNIPPETSELECTOR_

#include <QWidget>
#include "katesnippetglobal.h"

class KateView;
class KToolBar;

class KateCombinedSnippetSelector : public QWidget {
    Q_OBJECT
  
  public:
    KateCombinedSnippetSelector(QWidget *parent,KateView* initialView);
    virtual ~KateCombinedSnippetSelector();
    void createWidget();

  public Q_SLOTS:
    void updateSnippetsMode();
    
  private:
    QWidget *m_implementationWidget;
    KToolBar *m_toolBar;
    KateSnippetGlobal::Mode m_mode;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;