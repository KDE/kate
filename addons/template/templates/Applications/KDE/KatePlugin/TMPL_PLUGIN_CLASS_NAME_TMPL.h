#pragma once

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>

#include <QPointer>

class TMPL_PLUGIN_CLASS_NAME_TMPL : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit TMPL_PLUGIN_CLASS_NAME_TMPL(QObject *parent);

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class TMPL_PLUGIN_CLASS_NAME_TMPLView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit TMPL_PLUGIN_CLASS_NAME_TMPLView(TMPL_PLUGIN_CLASS_NAME_TMPL *plugin, KTextEditor::MainWindow *mainwindow);
    ~TMPL_PLUGIN_CLASS_NAME_TMPLView();

    void openTestDialog();
    void displayMessage(const QString &msg, KTextEditor::Message::MessageType level);

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QPointer<KTextEditor::Message> m_infoMessage;
};
