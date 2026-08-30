#pragma once

#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>

class GitDecorationsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit GitDecorationsPlugin(QObject *parent);
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    void annotateView(KTextEditor::View *view);

private:
    void registerDocument(KTextEditor::Document *document);
    void annotateDocument(KTextEditor::Document *document);
};

class GitDecorationsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow);

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
};
