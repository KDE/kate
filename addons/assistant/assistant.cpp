/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistant.h"

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

K_PLUGIN_FACTORY_WITH_JSON(AssistantFactory, "plugin.json", registerPlugin<Assistant>();)

Assistant::Assistant(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
}

QObject *Assistant::createView(KTextEditor::MainWindow *mainWindow)
{
    return new AssistantView(this, mainWindow);
}

AssistantView::AssistantView(Assistant *, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName(u"assistant"_s, i18n("Assistant"));
    setXMLFile(u"ui.rc"_s);

    QAction *a = actionCollection()->addAction(u"assistant_test_action"_s);
    a->setText(i18n("Test Action"));
    a->setIcon(QIcon::fromTheme(u"document-new-from-template"_s));
    connect(a, &QAction::triggered, this, &AssistantView::openTestDialog);

    m_mainWindow->guiFactory()->addClient(this);
}

AssistantView::~AssistantView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

void AssistantView::openTestDialog()
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

void AssistantView::displayMessage(const QString &msg, KTextEditor::Message::MessageType level)
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

#include "assistant.moc"
