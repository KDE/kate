#include <kcm_ktexteditorchooser.h>

KCMKTextEditorChooser::KCMKTextEditorChooser( QWidget *parent, const char *name ):
	KCModule(parent,name) {

	m_chooser=new KTextEditor::EditorChooser(this,"EditorChooser",KTextEditor::EditorChooser::SysSetting);
}
 
void KCMKTextEditorChooser::load(){
;
}

void KCMKTextEditorChooser::save(){
	m_chooser->writeSysDefault();
}

void KCMKTextEditorChooser::defaults(){
;
}

extern "C"
{
    KCModule *create_ktexteditorchooser( QWidget *parent, const char * )
    {
        return new KCMKTextEditorChooser( parent, "kcmktexteditor" );
    }
}

