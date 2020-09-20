/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_FILETREE_PLUGIN_H
#define KATE_FILETREE_PLUGIN_H

#include <QIcon>

#include <KTextEditor/Command>
#include <KTextEditor/Plugin>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/sessionconfiginterface.h>

#include "katefiletreepluginsettings.h"

#include <KXMLGUIClient>

class KToolBar;

class KateFileTree;
class KateFileTreeModel;
class KateFileTreeProxyModel;
class KateFileTreeConfigPage;
class KateFileTreePluginView;

class KateFileTreePlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateFileTreePlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KateFileTreePlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    const KateFileTreePluginSettings &settings();

    void applyConfig(bool shadingEnabled, const QColor &viewShade, const QColor &editShade, bool listMode, int sortRole, bool showFulPath);

public Q_SLOTS:
    void viewDestroyed(QObject *view);

private:
    QList<KateFileTreePluginView *> m_views;
    KateFileTreeConfigPage *m_confPage = nullptr;
    KateFileTreePluginSettings m_settings;
};

class KateFileTreePluginView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT

    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
    /**
     * Constructor.
     */
    KateFileTreePluginView(KTextEditor::MainWindow *mainWindow, KateFileTreePlugin *plug);

    /**
     * Virtual destructor.
     */
    ~KateFileTreePluginView() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

    /**
     * The file tree model.
     * @return the file tree model
     */
    KateFileTreeModel *model();
    /**
     * The file tree proxy model.
     * @return the file tree proxy model
     */
    KateFileTreeProxyModel *proxy();
    /**
     * The file tree.
     * @return the file tree
     */
    KateFileTree *tree();

    void setListMode(bool listMode);

    bool hasLocalPrefs();
    void setHasLocalPrefs(bool);

protected:
    void setupActions();

private:
    QWidget *m_toolView;
    KToolBar *m_toolbar;
    KateFileTree *m_fileTree;
    KateFileTreeProxyModel *m_proxyModel;
    KateFileTreeModel *m_documentModel;
    bool m_hasLocalPrefs = false;
    bool m_loadingDocuments;
    KateFileTreePlugin *m_plug;
    KTextEditor::MainWindow *m_mainWindow;

private Q_SLOTS:
    void showToolView();
    void hideToolView();
    void showActiveDocument();
    void activateDocument(KTextEditor::Document *);
    void viewChanged(KTextEditor::View * = nullptr);
    void documentOpened(KTextEditor::Document *);
    void documentClosed(KTextEditor::Document *);
    void viewModeChanged(bool);
    void sortRoleChanged(int);
    void slotAboutToCreateDocuments();
    void slotDocumentsCreated(const QList<KTextEditor::Document *> &);
    void slotDocumentSave();
    void slotDocumentSaveAs();
};

#endif // KATE_FILETREE_PLUGIN_H
