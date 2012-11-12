#ifndef _KATE_COMBINED_SNIPPETSELECTOR_
#define _KATE_COMBINED_SNIPPETSELECTOR_

#include <QWidget>
#include "katesnippetglobal.h"

class KateView;
class KToolBar;

class KateCombinedSnippetSelector : public QWidget {
    Q_OBJECT
  
  public:
    Q_PROPERTY (KTextEditor::View* currentEditorView WRITE setCurrentEditorView)
    
    KateCombinedSnippetSelector(QWidget *parent,KateView* initialView);
    virtual ~KateCombinedSnippetSelector();
    void createWidget(KateView *initialView);

  public Q_SLOTS:
    void updateSnippetsMode();
    void setCurrentEditorView(KTextEditor::View* view);
       
  Q_SIGNALS:
    void viewChanged(KateView* view);
    
  private:
    QWidget *m_implementationWidget;
    KToolBar *m_toolBar;
    KateSnippetGlobal::Mode m_mode;
    QPointer<KateView> m_currentEditorView;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;