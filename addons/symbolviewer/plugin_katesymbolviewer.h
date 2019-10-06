/***************************************************************************
                          plugin_katesymbolviewer.h  -  description
                             -------------------
    begin                : Apr 2 2003
    author               : 2003 Massimo Callegari
    email                : massimocallegari@yahoo.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PLUGIN_KATE_SYMBOLVIEWER_H_
#define _PLUGIN_KATE_SYMBOLVIEWER_H_

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>

#include <QCheckBox>
#include <QMenu>

#include <QLabel>
#include <QList>
#include <QPixmap>
#include <QResizeEvent>
#include <QSet>
#include <QTimer>
#include <QTreeWidget>

#include <klocalizedstring.h>

/**
 * Plugin's config page
 */
class KatePluginSymbolViewerConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

    friend class KatePluginSymbolViewer;

public:
    explicit KatePluginSymbolViewerConfigPage(QObject *parent = nullptr, QWidget *parentWidget = nullptr);
    ~KatePluginSymbolViewerConfigPage() override;

    /**
     * Reimplemented from KTextEditor::ConfigPage
     * just emits configPageApplyRequest( this ).
     */
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset() override
    {
        ;
    }
    void defaults() override
    {
        ;
    }

Q_SIGNALS:
    /**
     * Ask the plugin to set initial values
     */
    void configPageApplyRequest(KatePluginSymbolViewerConfigPage *);

    /**
     * Ask the plugin to apply changes
     */
    void configPageInitRequest(KatePluginSymbolViewerConfigPage *);

private:
    QCheckBox *viewReturns;
    QCheckBox *expandTree;
    QCheckBox *treeView;
    QCheckBox *sortSymbols;
};

class KatePluginSymbolViewer;

class KatePluginSymbolViewerView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    friend class KatePluginSymbolViewer;

public:
    KatePluginSymbolViewerView(KatePluginSymbolViewer *plugin, KTextEditor::MainWindow *mw);
    ~KatePluginSymbolViewerView() override;

public Q_SLOTS:
    void displayOptionChanged();
    void parseSymbols();
    void slotDocChanged();
    void goToSymbol(QTreeWidgetItem *);
    void slotShowContextMenu(const QPoint &);
    void cursorPositionChanged();
    QTreeWidgetItem *newActveItem(int &currMinLine, int currLine, QTreeWidgetItem *item);
    void updateCurrTreeItem();
    void slotDocEdited();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    KTextEditor::MainWindow *m_mainWindow;
    KatePluginSymbolViewer *m_plugin;
    QMenu *m_popup;
    QWidget *m_toolview;
    QTreeWidget *m_symbols;
    QAction *m_treeOn; // FIXME Rename other actions accordingly
    QAction *m_sort;   // m_sortOn etc
    QAction *m_macro;
    QAction *m_struct;
    QAction *m_func;
    QAction *m_typesOn;
    QAction *m_expandOn;

    QTimer m_updateTimer;
    QTimer m_currItemTimer;
    int m_oldCursorLine;

    void updatePixmapScroll();

    void parseCppSymbols(void);
    void parseTclSymbols(void);
    void parseFortranSymbols(void);
    void parsePerlSymbols(void);
    void parsePythonSymbols(void);
    void parseRubySymbols(void);
    void parseXsltSymbols(void);
    void parseXMLSymbols(void);
    void parsePhpSymbols(void);
    void parseBashSymbols(void);
    void parseEcmaSymbols(void);
};

class KatePluginSymbolViewer : public KTextEditor::Plugin
{
    friend class KatePluginSymbolViewerView;

    Q_OBJECT
public:
    explicit KatePluginSymbolViewer(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KatePluginSymbolViewer() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override
    {
        return 1;
    }
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

public Q_SLOTS:
    void applyConfig(KatePluginSymbolViewerConfigPage *p);

private:
    QSet<KatePluginSymbolViewerView *> m_views;
};

// icons
#include "icons.xpm"

#endif
