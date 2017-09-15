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

#include <KTextEditor/MainWindow>
#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/ConfigPage>

#include <QMenu>
#include <QCheckBox>

#include <QPixmap>
#include <QLabel>
#include <QResizeEvent>
#include <QTreeWidget>
#include <QList>
#include <QTimer>

#include <klocalizedstring.h>

/**
 * Plugin's config page
 */
class KatePluginSymbolViewerConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  friend class KatePluginSymbolViewer;

  public:
    explicit KatePluginSymbolViewerConfigPage (QObject* parent = nullptr, QWidget *parentWidget = nullptr);
    ~KatePluginSymbolViewerConfigPage () override;

    /**
     * Reimplemented from KTextEditor::ConfigPage
     * just emits configPageApplyRequest( this ).
     */
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset () override { ; }
    void defaults () override { ; }

  Q_SIGNALS:
    /**
     * Ask the plugin to set initial values
     */
    void configPageApplyRequest( KatePluginSymbolViewerConfigPage* );

    /**
     * Ask the plugin to apply changes
     */
    void configPageInitRequest( KatePluginSymbolViewerConfigPage* );

  private:
    QCheckBox* viewReturns;
    QCheckBox* expandTree;
    QCheckBox* treeView;
    QCheckBox* sortSymbols;
};

class KatePluginSymbolViewer;

class KatePluginSymbolViewerView :  public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    KatePluginSymbolViewerView (KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw);
    ~KatePluginSymbolViewerView () override;

    void parseSymbols(void);

  public Q_SLOTS:
    void slotRefreshSymbol();
    void slotChangeMode();
    void slotEnableSorting();
    void slotDocChanged();
    void goToSymbol(QTreeWidgetItem *);
    void slotShowContextMenu(const QPoint&);
    void toggleShowMacros(void);
    void toggleShowStructures(void);
    void toggleShowFunctions(void);
    void cursorPositionChanged();
    QTreeWidgetItem *newActveItem(int &currMinLine, int currLine, QTreeWidgetItem *item);
    void updateCurrTreeItem();
    void slotDocEdited();

  protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

  private:
    KTextEditor::MainWindow *m_mainWindow;
    KatePluginSymbolViewer *m_plugin;
    QMenu       *m_popup;
    QWidget     *m_toolview;
    QTreeWidget *m_symbols;
    QAction *m_macro, *m_struct, *m_func, *m_sort;
    bool macro_on, struct_on, func_on;

    QTimer m_updateTimer;
    QTimer m_currItemTimer;

    void updatePixmapScroll();

    void parseCppSymbols(void);
    void parseTclSymbols(void);
    void parseFortranSymbols(void);
    void parsePerlSymbols(void);
    void parsePythonSymbols(void);
    void parseRubySymbols(void);
    void parseXsltSymbols(void);
    void parsePhpSymbols(void);
    void parseBashSymbols(void);
    void parseEcmaSymbols(void);

};

class KatePluginSymbolViewer : public KTextEditor::Plugin
{
  Q_OBJECT
  public:
    explicit KatePluginSymbolViewer(QObject* parent = nullptr, const QList<QVariant>& = QList<QVariant>());
    ~KatePluginSymbolViewer() override;

    QObject *createView (KTextEditor::MainWindow *mainWindow) override;

    int configPages () const override { return 1; }
    KTextEditor::ConfigPage *configPage (int number = 0, QWidget *parent = nullptr) override;

  public Q_SLOTS:
    void applyConfig( KatePluginSymbolViewerConfigPage* p );

  public:
    bool typesOn;
    bool expandedOn;
    bool treeOn;
    bool sortOn;
};

/* XPM */
static const char* const class_xpm[] = {
"16 16 10 1",
" 	c None",
".	c #000000",
"+	c #A4E8FC",
"@	c #24D0FC",
"#	c #001CD0",
"$	c #0080E8",
"%	c #C0FFFF",
"&	c #00FFFF",
"*	c #008080",
"=	c #00C0C0",
"     ..         ",
"    .++..       ",
"   .+++@@.      ",
"  .@@@@@#...    ",
"  .$$@@##.%%..  ",
"  .$$$##.%%%&&. ",
"  .$$$#.&&&&&*. ",
"   ...#.==&&**. ",
"   .++..===***. ",
"  .+++@@.==**.  ",
" .@@@@@#..=*.   ",
" .$$@@##. ..    ",
" .$$$###.       ",
" .$$$##.        ",
"  ..$#.         ",
"    ..          "};

static const char * const class_int_xpm[] = {
"16 16 10 1",
" 	c None",
".	c #000000",
"+	c #B8B8B8",
"@	c #8A8A8A",
"#	c #212121",
"$	c #575757",
"%	c #CCCCCC",
"&	c #9A9A9A",
"*	c #4D4D4D",
"=	c #747474",
"     ..         ",
"    .++..       ",
"   .+++@@.      ",
"  .@@@@@#...    ",
"  .$$@@##.%%..  ",
"  .$$$##.%%%&&. ",
"  .$$$#.&&&&&*. ",
"   ...#.==&&**. ",
"   .++..===***. ",
"  .+++@@.==**.  ",
" .@@@@@#..=*.   ",
" .$$@@##. ..    ",
" .$$$###.       ",
" .$$$##.        ",
"  ..$#.         ",
"    ..          "};

static const char* const struct_xpm[] = {
"16 16 14 1",
" 	c None",
".	c #000000",
"+	c #C0FFC0",
"@	c #00FF00",
"#	c #008000",
"$	c #00C000",
"%	c #C0FFFF",
"&	c #00FFFF",
"*	c #008080",
"=	c #00C0C0",
"-	c #FFFFC0",
";	c #FFFF00",
">	c #808000",
",	c #C0C000",
"     ..         ",
"    .++..       ",
"   .+++@@.      ",
"  .@@@@@#...    ",
"  .$$@@##.%%..  ",
"  .$$$##.%%%&&. ",
"  .$$$#.&&&&&*. ",
"   ...#.==&&**. ",
"   .--..===***. ",
"  .---;;.==**.  ",
" .;;;;;>..=*.   ",
" .,,;;>>. ..    ",
" .,,,>>>.       ",
" .,,,>>.        ",
"  ..,>.         ",
"    ..          "};

static const char* const macro_xpm[] = {
"16 16 14 1",
" 	c None",
".	c #000000",
"+	c #FF7FE5",
"@	c #FF00C7",
"#	c #7F0066",
"$	c #BC0096",
"%	c #C0FFFF",
"&	c #00FFFF",
"*	c #008080",
"=	c #00C0C0",
"-	c #D493FF",
";	c #A100FF",
">	c #470082",
",	c #6B00B7",
"     ..         ",
"    .++..       ",
"   .+++@@.      ",
"  .@@@@@#...    ",
"  .$$@@##.%%..  ",
"  .$$$##.%%%&&. ",
"  .$$$#.&&&&&*. ",
"   ...#.==&&**. ",
"   .--..===***. ",
"  .---;;.==**.  ",
" .;;;;;>..=*.   ",
" .,,;;>>. ..    ",
" .,,,>>>.       ",
" .,,,>>.        ",
"  ..,>.         ",
"    ..          "};

static const char* const method_xpm[] = {
  "16 16 5 1",
  "       c None",
  ".      c #000000",
  "+      c #FCFC80",
  "@      c #E0BC38",
  "#      c #F0DC5C",
  "                ",
  "                ",
  "                ",
  "          ..    ",
  "         .++..  ",
  "        .+++++. ",
  "       .+++++@. ",
  "    .. .##++@@. ",
  "   .++..###@@@. ",
  "  .+++++.##@@.  ",
  " .+++++@..#@.   ",
  " .##++@@. ..    ",
  " .###@@@.       ",
  " .###@@.        ",
  "  ..#@.         ",
  "    ..          "
};

#endif
