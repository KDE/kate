#ifndef _EDITOR_CHOOSER_H_
#define  _EDITOR_CHOOSER_H_

#include <editorchooser_ui.h>

class KConfig;
class QString;

namespace KTextEditor
{

class EditorChooser: public EditorChooser_UI {

Q_OBJECT

public:
	enum ChooserType {AppSetting=0,SysSetting};
	EditorChooser(QWidget *parent=0,const char *name=0,const ChooserType type=AppSetting);
	virtual ~EditorChooser();
	void writeSysDefault();
	void readAppSetting(KConfig *cfg,const QString& postfix);
	void writeAppSetting(KConfig *cfg,const QString& postfix);
};
};
#endif
