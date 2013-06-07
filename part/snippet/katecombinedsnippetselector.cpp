#include "katecombinedsnippetselector.h"
#include "katecombinedsnippetselector.moc"

#include "katesnippetglobal.h"
#include "kateglobal.h"
#include "snippetview.h"
#include "categorizedsnippetselector.h"

#include <KToolBar>

#include <QVBoxLayout>


KateCombinedSnippetSelector::KateCombinedSnippetSelector(QWidget *parent,KateView* initialView):
  QWidget(parent), m_implementationWidget(0) {
    KateSnippetGlobal *global=KateSnippetGlobal::self();
    m_mode=global->snippetsMode();
    createWidget();
    connect(global,SIGNAL(snippetModeChanged()),this,SLOT(updateSnippetsMode()));
  }

KateCombinedSnippetSelector::~KateCombinedSnippetSelector() { m_implementationWidget=0;}

void KateCombinedSnippetSelector::updateSnippetsMode() {
  enum KateSnippetGlobal::Mode m=KateSnippetGlobal::self()->snippetsMode();
  if (m!=m_mode) {
    m_mode=m;
    createWidget();
  }
}

void KateCombinedSnippetSelector::createWidget() {
    KateSnippetGlobal *global=KateSnippetGlobal::self();
    delete m_toolBar;
    m_toolBar=0;
    delete m_implementationWidget;
    m_implementationWidget=0;
    QLayout *l=layout();
    setLayout(0);
    delete l;
    switch (m_mode) {
      case KateSnippetGlobal::FileModeBasedMode: {
          QVBoxLayout *layout=new QVBoxLayout(this);
          m_implementationWidget=new JoWenn::KateCategorizedSnippetSelector(this);
          layout->addWidget(m_implementationWidget);
        }     
        break;

      case KateSnippetGlobal::SnippetFileBasedMode: {
          QVBoxLayout *layout=new QVBoxLayout(this);
          m_toolBar=new KToolBar(this,"snippetsToolBar");
          m_toolBar->setToolButtonStyle (Qt::ToolButtonIconOnly);
          layout->addWidget(m_toolBar);
          m_implementationWidget=new SnippetView(global,this);
          layout->addWidget(m_implementationWidget);
          m_toolBar->addActions(m_implementationWidget->actions());
        }
        break;
        
      default:
        break; //error case
    }
}


// kate: space-indent on; indent-width 2; replace-tabs on;