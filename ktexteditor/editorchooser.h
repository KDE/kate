#ifndef _EDITOR_CHOOSER_H_
#define  _EDITOR_CHOOSER_H_

#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>

#include <qwidget.h>

class KConfig;
class QString;

namespace KTextEditor
{

class EditorChooser: public QWidget
{                    
  friend class PrivateEditorChooser;

  Q_OBJECT

  public:
    EditorChooser(QWidget *parent=0,const char *name=0);
    virtual ~EditorChooser();
    
    void writeSysDefault();

    void readAppSetting(const QString& postfix=QString::null);
    void writeAppSetting(const QString& postfix=QString::null);

    static KTextEditor::Document *createDocument(QObject* parent=0,const char *name=0,const QString& postfix=QString::null, bool fallBackToKatePart=true);
    static KTextEditor::Editor *createEditor(const QString& postfix=QString::null, bool fallBackToKatePart=true);
 
  private:
    class PrivateEditorChooser *d;
};

/*
class EditorChooserBackEnd: public ComponentChooserPlugin {

Q_OBJECT
public:
	EditorChooserBackEnd(QObject *parent=0, const char *name=0);
	virtual ~EditorChooserBackEnd();

	virtual QWidget *widget(QWidget *);
	virtual const QStringList &choices();
	virtual void saveSettings();

	void readAppSetting(KConfig *cfg,const QString& postfix);
	void writeAppSetting(KConfig *cfg,const QString& postfix);

public slots:
	virtual void madeChoice(int pos,const QString &choice);

};
*/

};
#endif
