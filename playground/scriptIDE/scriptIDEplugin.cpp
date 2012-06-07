/*   Kate Script IDE
 */

#include "scriptIDEplugin.h"
#include "scriptIDEplugin.moc"


#include <kate/application.h>
#include <ktexteditor/editor.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/configinterface.h>

#include <kfiledialog.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kurlcompletion.h>
#include <klineedit.h>
#include <QKeyEvent>
#include <QClipboard>
#include <QMenu>
#include <QTextDocument>
#include <QtScript>
#include <QFile>
#include <QUrl>


K_PLUGIN_FACTORY(KatePluginScriptIDEFactory, registerPlugin<KatePluginScriptIDE>();)
K_EXPORT_PLUGIN(KatePluginScriptIDEFactory(KAboutData("scriptIDE","scriptIDE",ki18n("Script IDE"), "0.1", ki18n("Plugin for Script Management"))))

KatePluginScriptIDE::KatePluginScriptIDE(QObject* parent, const QList<QVariant>&)
    : Kate::Plugin((Kate::Application*)parent, "scriptIDE")
{
    KGlobal::locale()->insertCatalog("scriptIDE");
}

Kate::PluginView *KatePluginScriptIDE::createView(Kate::MainWindow *mainWindow)
{
    KatePluginScriptIDEView *view = new KatePluginScriptIDEView(mainWindow, application());
    return view;
}

KatePluginScriptIDEView::KatePluginScriptIDEView(Kate::MainWindow *mainWin, Kate::Application* application)
: Kate::PluginView(mainWin),
Kate::XMLGUIClient(KatePluginScriptIDEFactory::componentData()),
m_kateApp(application)
{
    
    m_toolView = mainWin->createToolView ("kate_plugin_scriptIDE",
                                          Kate::MainWindow::Bottom,
                                          SmallIcon("application-javascript"),
                                          i18n("Script IDE"));
    QWidget *container = new QWidget(m_toolView);
    m_ui.setupUi(container);
    
    connect(m_ui.btnEvaluate, SIGNAL(clicked()), this, SLOT(evaluate()));
    connect(m_ui.btnOpenFile, SIGNAL(clicked()), this, SLOT(openFile()));
    
    mainWindow()->guiFactory()->addClient(this);
}

void KatePluginScriptIDEView::evaluate()
{
    QScriptEngine engine;
  
    
//     Bindings bindings;
//     QScriptValue scriptBindings = engine.newQObject(&bindings);
//     engine.globalObject().setProperty("ext", scriptBindings);
    if (m_ui.inputField->toPlainText()!= ""){
        m_ui.answerField->setPlainText(engine.evaluate(m_ui.inputField->toPlainText()).toString());
    }
    else{
      m_ui.answerField->setPlainText("Nothing to evaluate!");
    }
}

void KatePluginScriptIDEView::openFile()
{
    QString filename = KFileDialog::getOpenFileName(QUrl("~"), "*.js");
    
    QFile file(filename);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return;

     QTextStream in(&file);
     while (!in.atEnd()) {
         QString text = in.readAll();
         m_ui.inputField->setPlainText(text);
     }
}

KatePluginScriptIDEView::~KatePluginScriptIDEView()
{
    mainWindow()->guiFactory()->removeClient(this);
    delete m_toolView;
}

// Bindings::Bindings()
//   : QObject()
//   , QScriptable()
// {
// }
// 
// QString Bindings::schmuh()
// {
//     return "hello world!";
// }