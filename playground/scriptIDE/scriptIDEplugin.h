/*   Kate Script IDE
 */

#ifndef _PLUGIN_SCRIPTIDE_H_
#define _PLUGIN_SCRIPTIDE_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/commandinterface.h>
#include <kaction.h>

#include "ui_scriptIDE.h"

#include <QTreeWidget>
#include <QScriptable>


class KatePluginScriptIDE : public Kate::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginScriptIDE(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());

    Kate::PluginView *createView(Kate::MainWindow *mainWindow);
    
};

class KatePluginScriptIDEView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT
    
public slots:
    void evaluate();
    void openFile();

public:
    KatePluginScriptIDEView(Kate::MainWindow *mainWindow, Kate::Application* application);
    ~KatePluginScriptIDEView();
    
private:
    Ui::ScriptDialog                  m_ui;
    QWidget                           *m_toolView;
    Kate::Application                 *m_kateApp;

};

// class Bindings : public QObject, public QScriptable
// {
//     Q_OBJECT
//     
// public:
//     Bindings();
//     
//     Q_INVOKABLE QString schmuh();
// };

#endif
