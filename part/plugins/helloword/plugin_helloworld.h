#ifndef __plugin_spellcheck_h
#define __plugin_spellcheck_h

#include <kparts/plugin.h>
#include <klibloader.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>

class HelloWorld : public KTextEditor::ViewPlugin
{
    Q_OBJECT
public:
    HelloWorld( QObject* parent = 0, const char* name = 0 );
    virtual ~HelloWorld();

    void setView (KTextEditor::View *view) {};
    
public slots:
    void slotSpellCheck();

};

class KPluginFactory : public KLibFactory
{
    Q_OBJECT
public:
    KPluginFactory( QObject* parent = 0, const char* name = 0 );
    ~KPluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

private:
    static KInstance* s_instance;
};

#endif
