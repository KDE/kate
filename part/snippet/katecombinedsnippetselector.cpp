#include "katecombinedsnippetselector.h"
#include "katecombinedsnippetselector.moc"

#include "katesnippetglobal.h"
#include "kateglobal.h"
#include "snippetview.h"
#include "categorizedsnippetselector.h"

#include <KToolBar>

#include <QVBoxLayout>


KateCombinedSnippetSelector::KateCombinedSnippetSelector(QWidget *parent,KateView* initialView):
  QWidget(parent), m_implementationWidget(0),m_toolBar(0) {
    KateSnippetGlobal *global=KateSnippetGlobal::self();
    m_mode=global->snippetsMode();
    createWidget(initialView);
    connect(global,SIGNAL(snippetsModeChanged()),this,SLOT(updateSnippetsMode()));
  }

KateCombinedSnippetSelector::~KateCombinedSnippetSelector() { m_implementationWidget=0;}

void KateCombinedSnippetSelector::updateSnippetsMode() {
  enum KateSnippetGlobal::Mode m=KateSnippetGlobal::self()->snippetsMode();
  if (m!=m_mode) {
    m_mode=m;
    if (!m_currentEditorView) 
      setCurrentEditorView(KateSnippetGlobal::self()->getCurrentView());
    createWidget(m_currentEditorView);
  }
}

void KateCombinedSnippetSelector::createWidget(KateView *initialView) {
    KateSnippetGlobal *global=KateSnippetGlobal::self();
    delete m_toolBar;
    m_toolBar=0;
    delete m_implementationWidget;
    m_implementationWidget=0;
    QLayout *l=layout();
    setLayout(0);
    setCurrentEditorView(initialView);
    delete l;
    switch (m_mode) {
      case KateSnippetGlobal::FileModeBasedMode: {
          QVBoxLayout *layout=new QVBoxLayout(this);
          m_implementationWidget=new JoWenn::KateCategorizedSnippetSelector(this);
          layout->addWidget(m_implementationWidget);
          connect(this,SIGNAL(viewChanged(KateView*)),m_implementationWidget,SLOT(viewChanged(KateView*)));
          viewChanged(initialView);
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


void KateCombinedSnippetSelector::setCurrentEditorView(KTextEditor::View* view) {
  KateView* kateView=qobject_cast<KateView*>(view);
  if (!kateView) {
    m_currentEditorView=0;
    emit viewChanged(0);
  } else {
    if (kateView!=m_currentEditorView) {
      m_currentEditorView=kateView;
      emit viewChanged(kateView);
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;