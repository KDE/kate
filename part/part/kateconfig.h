/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_CONFIG_H__
#define __KATE_CONFIG_H__

#include <qobject.h>

class KateView;
class KateDocument;
class KateRenderer;
class FontStruct;

class KConfig;

class QColor;
class QFont;
class QTextCodec;
class KateFontMetrics;

class KateConfig
{
  public:
    KateConfig ();
    virtual ~KateConfig ();

  public:
     void configStart ();
     void configEnd ();

  protected:
    virtual void updateConfig () = 0;

  private:
    uint configSessionNumber;
    bool configIsRunning;
};

class KateDocumentConfig : public KateConfig
{
  private:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateDocumentConfig ();

  public:
    /**
     * Construct a DocumentConfig
     */
    KateDocumentConfig (KateDocument *doc);

    /**
     * Cu DocumentConfig
     */
    ~KateDocumentConfig ();

    static KateDocumentConfig *global ();

    inline bool isGlobal () const { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

  protected:
    void updateConfig ();

  public:
    int tabWidth () const;
    void setTabWidth (int tabWidth);

    int indentationWidth () const;
    void setIndentationWidth (int indentationWidth);

    enum IndentationMode
    {
      imNormal = 0,
      imCStyle = 1,
      imPythonStyle = 2
    };

    uint indentationMode () const;
    void setIndentationMode (uint identationMode);

    bool wordWrap () const;
    void setWordWrap (bool on);

    unsigned int wordWrapAt () const;
    void setWordWrapAt (unsigned int col);

    uint undoSteps () const;
    void setUndoSteps ( uint undoSteps );

    bool pageUpDownMovesCursor () const;
    void setPageUpDownMovesCursor (bool on);

    enum ConfigFlags
    {
      cfAutoIndent= 0x1,
      cfBackspaceIndents= 0x2,
      cfWordWrap= 0x4,
      cfReplaceTabs= 0x8,
      cfRemoveSpaces = 0x10,
      cfWrapCursor= 0x20,
      cfAutoBrackets= 0x40,
      cfPersistent= 0x80,
      cfKeepSelection= 0x100,
      cfTabIndentsMode = 0x200,
      cfDelOnInput= 0x400,
      cfXorSelect= 0x800,
      cfOvr= 0x1000,
      cfMark= 0x2000,
      cfKeepIndentProfile= 0x8000,
      cfKeepExtraSpaces= 0x10000,
      cfTabIndents= 0x80000,
      cfShowTabs= 0x200000,
      cfSpaceIndent= 0x400000,
      cfSmartHome = 0x800000
    };

    uint configFlags () const;
    void setConfigFlags (KateDocumentConfig::ConfigFlags flag, bool enable);
    void setConfigFlags (uint fullFlags);

    const QString &encoding () const;
    QTextCodec *codec ();

    void setEncoding (const QString &encoding);

    enum Eol
    {
      eolUnix = 0,
      eolDos = 1,
      eolMac = 2
    };

    int eol () const;
    QString eolString ();

    void setEol (int mode);

    enum BackupFlags
    {
      LocalFiles=1,
      RemoteFiles=2
    };

    uint backupFlags () const;
    void setBackupFlags (uint flags);

    const QString &backupSuffix () const;
    void setBackupSuffix (const QString &suffix);

  private:
    int m_tabWidth;
    int m_indentationWidth;
    uint m_indentationMode;
    bool m_wordWrap;
    int m_wordWrapAt;
    uint m_undoSteps;
    bool m_pageUpDownMovesCursor;
    uint m_configFlags;
    QString m_encoding;
    int m_eol;
    uint m_backupFlags;
    QString m_backupSuffix;

    bool m_tabWidthSet : 1;
    bool m_indentationWidthSet : 1;
    bool m_indentationModeSet : 1;
    bool m_wordWrapSet : 1;
    bool m_wordWrapAtSet : 1;
    bool m_pageUpDownMovesCursorSet : 1;
    bool m_undoStepsSet : 1;
    uint m_configFlagsSet;
    bool m_encodingSet : 1;
    bool m_eolSet : 1;
    bool m_backupFlagsSet : 1;
    bool m_backupSuffixSet : 1;

  private:
    KateDocument *m_doc;

    static KateDocumentConfig *s_global;
};

class KateViewConfig : public KateConfig
{
  private:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateViewConfig ();

  public:
    /**
     * Construct a DocumentConfig
     */
    KateViewConfig (KateView *view);

    /**
     * Cu DocumentConfig
     */
    ~KateViewConfig ();

    static KateViewConfig *global ();

    inline bool isGlobal () const { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

  protected:
    void updateConfig ();

  public:
    bool dynWordWrap () const;
    void setDynWordWrap (bool wrap);

    int dynWordWrapIndicators () const;
    void setDynWordWrapIndicators (int mode);

    bool lineNumbers () const;
    void setLineNumbers (bool on);

    bool iconBar () const;
    void setIconBar (bool on);

    bool foldingBar () const;
    void setFoldingBar (bool on);

    int bookmarkSort () const;
    void setBookmarkSort (int mode);

    int autoCenterLines() const;
    void setAutoCenterLines (int lines);

    long searchFlags () const;
    void setSearchFlags (long flags);

    bool cmdLine () const;
    void setCmdLine (bool on);

  private:
    bool m_dynWordWrap;
    int m_dynWordWrapIndicators;
    bool m_lineNumbers;
    bool m_iconBar;
    bool m_foldingBar;
    int m_bookmarkSort;
    int m_autoCenterLines;
    long m_searchFlags;
    bool m_cmdLine;

    bool m_dynWordWrapSet : 1;
    bool m_dynWordWrapIndicatorsSet : 1;
    bool m_lineNumbersSet : 1;
    bool m_iconBarSet : 1;
    bool m_foldingBarSet : 1;
    bool m_bookmarkSortSet : 1;
    bool m_autoCenterLinesSet : 1;
    bool m_searchFlagsSet : 1;
    bool m_cmdLineSet : 1;

  private:
    KateView *m_view;

    static KateViewConfig *s_global;
};

class KateRendererConfig : public KateConfig
{
  private:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateRendererConfig ();

  public:
    /**
     * Construct a DocumentConfig
     */
    KateRendererConfig (KateRenderer *renderer);

    /**
     * Cu DocumentConfig
     */
    ~KateRendererConfig ();

    static KateRendererConfig *global ();

    inline bool isGlobal () const { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

  protected:
    void updateConfig ();

  public:
    uint schema () const;
    void setSchema (uint schema);

    FontStruct *fontStruct ();
    QFont *font();
    KateFontMetrics *fontMetrics();

    void setFont(const QFont &font);

    bool wordWrapMarker () const;
    void setWordWrapMarker (bool on);

    const QColor *backgroundColor() const;
    void setBackgroundColor (const QColor &col);

    const QColor *selectionColor() const;
    void setSelectionColor (const QColor &col);

    const QColor *highlightedLineColor() const;
    void setHighlightedLineColor (const QColor &col);

    const QColor *highlightedBracketColor() const;
    void setHighlightedBracketColor (const QColor &col);

    const QColor *wordWrapMarkerColor() const;
    void setWordWrapMarkerColor (const QColor &col);

    const QColor *tabMarkerColor() const;
    void setTabMarkerColor (const QColor &col);

    const QColor *iconBarColor() const;
    void setIconBarColor (const QColor &col);

  private:
    uint m_schema;
    FontStruct* m_font;
    bool m_wordWrapMarker;
    QColor *m_backgroundColor;
    QColor *m_selectionColor;
    QColor *m_highlightedLineColor;
    QColor *m_highlightedBracketColor;
    QColor *m_wordWrapMarkerColor;
    QColor *m_tabMarkerColor;
    QColor *m_iconBarColor;

    bool m_schemaSet : 1;
    bool m_fontSet : 1;
    bool m_wordWrapMarkerSet : 1;
    bool m_backgroundColorSet : 1;
    bool m_selectionColorSet : 1;
    bool m_highlightedLineColorSet : 1;
    bool m_highlightedBracketColorSet : 1;
    bool m_wordWrapMarkerColorSet : 1;
    bool m_tabMarkerColorSet : 1;
    bool m_iconBarColorSet : 1;

  private:
    KateRenderer *m_renderer;

    static KateRendererConfig *s_global;
};

#endif
