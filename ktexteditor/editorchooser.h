#ifndef _EDITOR_CHOOSER_H_
#define  _EDITOR_CHOOSER_H_

#include <editorchooser_ui.h>

namespace KTextEditor
{

class EditorChooser: public EditorChooser_UI {

Q_OBJECT

public:
	EditorChooser(QWidget *parent=0,const char *name=0);
	virtual ~EditorChooser();
	void writeSysDefault();
//	readAppSetting(KConfig *cfg,const QString& prefix);
//	writeAppSetting(KConfig *cfg,const QString& prefix);
};
};
#endif
