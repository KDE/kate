#ifndef _KCM_KTEXTEDITORCHOOSER_H_
#define _KCM_KTEXTEDITORCHOOSER_H_

#include <kcmodule.h>
#include <editorchooser.h>

class KCMKTextEditorChooser : public KCModule
{
    Q_OBJECT
public:
    KCMKTextEditorChooser( QWidget *parent = 0, const char *name = 0 );
 
    void load();
    void save();
    void defaults();

private:
    KTextEditor::EditorChooser  *m_chooser;
};

#endif
