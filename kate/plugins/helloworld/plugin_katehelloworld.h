
#ifndef _PLUGIN_KATE_HELLOWORLD_H_
#define _PLUGIN_KATE_HELLOWORLD_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kxmlguiclient.h>

class KatePluginHelloWorld : public Kate::Plugin
{
  Q_OBJECT

  public:
    explicit KatePluginHelloWorld( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KatePluginHelloWorld();

    Kate::PluginView *createView( Kate::MainWindow *mainWindow );

  public slots:
    void slotInsertHello();
};

class KatePluginHelloWorldView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

  public:
    KatePluginHelloWorldView( Kate::MainWindow *mainWindow );
    ~KatePluginHelloWorldView();

    virtual void readSessionConfig( KConfigBase* config, const QString& groupPrefix );
    virtual void writeSessionConfig( KConfigBase* config, const QString& groupPrefix );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

