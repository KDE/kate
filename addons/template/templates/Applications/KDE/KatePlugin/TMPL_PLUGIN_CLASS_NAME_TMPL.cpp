#include "TMPL_PLUGIN_CLASS_NAME_TMPL.h"

#include <KActionCollection>
#include <KPluginFactory>

#include <QAction>
#include <QFileInfo>
#include <QIcon>
#include <QLayout>
#include <QMessageBox>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

using namespace Qt::Literals::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(TMPL_PLUGIN_CLASS_NAME_TMPLFactory, "plugin.json", registerPlugin<TMPL_PLUGIN_CLASS_NAME_TMPL>();)

TMPL_PLUGIN_CLASS_NAME_TMPL::TMPL_PLUGIN_CLASS_NAME_TMPL(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

QObject *TMPL_PLUGIN_CLASS_NAME_TMPL::createView(KTextEditor::MainWindow *mainWindow)
{
    return new TMPL_PLUGIN_CLASS_NAME_TMPLView(this, mainWindow);
}

TMPL_PLUGIN_CLASS_NAME_TMPLView::TMPL_PLUGIN_CLASS_NAME_TMPLView(TMPL_PLUGIN_CLASS_NAME_TMPL *, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName(u"TMPL_PLUGIN_LIB_NAME_TMPL"_s, i18n("TMPL_PLUGIN_UI_NAME_TMPL"));
    setXMLFile(u"ui.rc"_s);

    QAction *a = actionCollection()->addAction(u"TMPL_PLUGIN_LIB_NAME_TMPL_test_action"_s);
    a->setText(i18n("Test Action"));
    a->setIcon(QIcon::fromTheme(u"document-new-from-template"_s));
    connect(a, &QAction::triggered, this, &TMPL_PLUGIN_CLASS_NAME_TMPLView::openTestDialog);

    m_mainWindow->guiFactory()->addClient(this);
}

TMPL_PLUGIN_CLASS_NAME_TMPLView::~TMPL_PLUGIN_CLASS_NAME_TMPLView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

void TMPL_PLUGIN_CLASS_NAME_TMPLView::openTestDialog()
{
    QMessageBox msgBox(m_mainWindow->window());
    msgBox.setWindowTitle(i18n("Test Dialog"));
    msgBox.setText(i18n("Test Dialog"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        displayMessage(i18n("You clicked Yes"), KTextEditor::Message::Information);
    } else {
        displayMessage(i18n("You clicked No"), KTextEditor::Message::Information);
    }
}

void TMPL_PLUGIN_CLASS_NAME_TMPLView::displayMessage(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_mainWindow->activeView();
    if (!kv) {
        return;
    }

    delete m_infoMessage;
    m_infoMessage = new KTextEditor::Message(msg, level);
    m_infoMessage->setWordWrap(false);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(8000);
    m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
    m_infoMessage->setView(kv);
    kv->document()->postMessage(m_infoMessage);
}

#include "TMPL_PLUGIN_CLASS_NAME_TMPL.moc"
