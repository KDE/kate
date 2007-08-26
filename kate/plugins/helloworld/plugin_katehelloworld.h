#ifndef _PLUGIN_KATE_HELLOWORLD_H_
#define _PLUGIN_KATE_HELLOWORLD_H_

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/view.h>

#include <QList>

class KatePluginHelloWorld : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    explicit KatePluginHelloWorld( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KatePluginHelloWorld();

    void storeGeneralConfig(KConfig* config,const QString& groupPrefix);
    void loadGeneralConfig(KConfig* config,const QString& groupPrefix);

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    void storeViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix);
    void loadViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix);

  public slots:
    void slotInsertHello();  
    
  private:
    QList<class PluginView*> m_views; 
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

