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

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/view.h>

#include <kdebug.h>
#include <QMenu>
#include <qevent.h>
#include <qcheckbox.h>

#include <QPixmap>
#include <QResizeEvent>
#include <QTreeWidget>
#include <QList>
#include <klibloader.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kcmodule.h>

class KatePluginSymbolViewerView;
class KatePluginSymbolViewerView2 : public Kate::PluginView
{
  Q_OBJECT

  public:
    KatePluginSymbolViewerView2 (Kate::MainWindow *w);
    virtual ~KatePluginSymbolViewerView2 ();
    KatePluginSymbolViewerView* view();
  private:
    KatePluginSymbolViewerView *m_view;
};

class KatePluginSymbolViewerView : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    KatePluginSymbolViewerView (Kate::MainWindow *w);
    virtual ~KatePluginSymbolViewerView ();

    void parseSymbols(void);

  public slots:
    void slotInsertSymbol();
    void slotRefreshSymbol();
    void slotChangeMode();
    void slotEnableSorting();
    void slotDocChanged();
    void goToSymbol(QTreeWidgetItem *);
    void slotShowContextMenu(const QPoint&);
    void toggleShowMacros(void);
    void toggleShowStructures(void);
    void toggleShowFunctions(void);
  protected:
    void slotViewChanged(QResizeEvent *e);
  private:
    QMenu *popup;
    QTreeWidget *symbols;
    QWidget *dock;
    bool m_Active;
    int m_macro, m_struct, m_func, m_sort;
    bool macro_on, struct_on, func_on;
    bool treeMode, lsorting;
    void parseCppSymbols(void);
    void parseTclSymbols(void);
    void parseFortranSymbols(void);
    void parsePerlSymbols(void);
    void parsePythonSymbols(void);
    void parseRubySymbols(void);
    void parseXsltSymbols(void);
    void parsePhpSymbols(void);
  public:
    Kate::MainWindow *win;
    bool types_on;
    bool expanded_on;
};

/**
 * Plugin's config page
 */
class KatePluginSymbolViewerConfigPage : public KCModule
{
  Q_OBJECT

  friend class KatePluginSymbolViewer;

  public:
    explicit KatePluginSymbolViewerConfigPage (QWidget* parent = 0L, const QVariantList& = QVariantList());
    ~KatePluginSymbolViewerConfigPage ();

    virtual void save();

    virtual void load () { ; }
    virtual void defaults () { ; }

  signals:
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
};

class KatePluginSymbolViewer : public Kate::Plugin
{
  Q_OBJECT
  public:
    explicit KatePluginSymbolViewer( QObject* parent = 0, const QVariantList& = QVariantList() );
    virtual ~KatePluginSymbolViewer();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
    void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
    void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
    void loadGeneralConfig(KConfig* config, const QString& groupPrefix);

  public slots:
    void applyConfig( KatePluginSymbolViewerConfigPage* );

  private:
    QList<KatePluginSymbolViewerView *> m_views;
    KConfig pConfig;
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
