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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_CONFIG_H__
#define __KATE_CONFIG_H__

#include <ktexteditor/markinterface.h>
#include <kencodingdetector.h>

#include <QtCore/QBitRef>
#include <QtGui/QColor>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtGui/QFontMetrics>

class KConfigGroup;
class KateView;
class KateDocument;
class KateRenderer;

class KConfig;

class QTextCodec;


/**
 * Base Class for the Kate Config Classes
 */
class KateConfig
{
  public:
    /**
     * Default Constructor
     */
    KateConfig ();

    /**
     * Virtual Destructor
     */
    virtual ~KateConfig ();

  public:
     /**
      * start some config changes
      * this method is needed to init some kind of transaction
      * for config changes, update will only be done once, at
      * configEnd() call
      */
     void configStart ();

     /**
      * end a config change transaction, update the concerned
      * documents/views/renderers
      */
     void configEnd ();

  protected:
    /**
     * do the real update
     */
    virtual void updateConfig () = 0;

  private:
    /**
     * recursion depth
     */
    uint configSessionNumber;

    /**
     * is a config session running
     */
    bool configIsRunning;
};

class KateDocumentConfig : public KateConfig
{
  private:
    friend class KateGlobal;

    /**
     * only used in KateGlobal for the static global fallback !!!
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

    inline static KateDocumentConfig *global () { return s_global; }

    inline bool isGlobal () const { return (this == global()); }

  public:
    /**
     * Read config from object
     */
    void readConfig (const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig (KConfigGroup &config);

  protected:
    void updateConfig ();

  public:
    int tabWidth () const;
    void setTabWidth (int tabWidth);

    int indentationWidth () const;
    void setIndentationWidth (int indentationWidth);

    const QString &indentationMode () const;
    void setIndentationMode (const QString &identationMode);

    enum TabHandling
    {
      tabInsertsTab = 0,
      tabIndents = 1,
      tabSmart = 2      //!< indents in leading space, otherwise inserts tab
    };

    uint tabHandling () const;
    void setTabHandling (uint tabHandling);

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
      cfBackspaceIndents= 0x2,
      cfWordWrap= 0x4,
      cfRemoveSpaces = 0x10,
      cfWrapCursor= 0x20,
      cfAutoBrackets= 0x40,
      cfTabIndentsMode = 0x200,
      cfOvr= 0x1000,
      cfKeepExtraSpaces= 0x10000,
      cfTabIndents= 0x80000,
      cfShowTabs= 0x200000,
      cfShowSpaces= 0x400000,
      cfSmartHome = 0x800000,
      cfTabInsertsTab = 0x1000000,
      cfReplaceTabsDyn=   0x2000000,
      cfRemoveTrailingDyn=0x4000000,
      cfIndentPastedText = 0x10000000
    };

    uint configFlags () const;
    void setConfigFlags (KateDocumentConfig::ConfigFlags flag, bool enable);
    void setConfigFlags (uint fullFlags);

    QTextCodec *codec () const;
    const QString &encoding () const;
    bool setEncoding (const QString &encoding,bool resetDetection=true);
    bool isSetEncoding () const;
    KEncodingDetector::AutoDetectScript encodingAutoDetectionScript() const;
    void setEncodingAutoDetectionScript(KEncodingDetector::AutoDetectScript);

    enum Eol
    {
      eolUnix = 0,
      eolDos = 1,
      eolMac = 2
    };

    int eol () const;
    QString eolString ();

    void setEol (int mode);

    bool allowEolDetection () const;
    void setAllowEolDetection (bool on);

    enum BackupFlags
    {
      LocalFiles=1,
      RemoteFiles=2
    };

    uint backupFlags () const;
    void setBackupFlags (uint flags);

    const QString &backupPrefix () const;
    void setBackupPrefix (const QString &prefix);

    const QString &backupSuffix () const;
    void setBackupSuffix (const QString &suffix);

    bool plugin (int index) const;
    void setPlugin (int index, bool load);

    /**
     * Should Kate Part search for dir wide config file
     * and if, how depth?
     * @return search depth (< 0 no search)
     */
    int searchDirConfigDepth () const;

    void setSearchDirConfigDepth (int depth);

  private:
    QString m_indentationMode;
    int m_indentationWidth;
    int m_tabWidth;
    uint m_tabHandling;
    uint m_configFlags;
    int m_wordWrapAt;
    bool m_wordWrap;
    bool m_pageUpDownMovesCursor;
    bool m_allowEolDetection;
    int m_eol;
    uint m_backupFlags;
    uint m_undoSteps;
    int m_searchDirConfigDepth;
    KEncodingDetector::AutoDetectScript m_scriptForEncodingAutoDetection;
    QString m_encoding;
    QString m_backupPrefix;
    QString m_backupSuffix;
    QBitArray m_plugins;

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
    bool m_allowEolDetectionSet : 1;
    bool m_backupFlagsSet : 1;
    bool m_searchDirConfigDepthSet : 1;
    bool m_backupPrefixSet : 1;
    bool m_backupSuffixSet : 1;
    QBitArray m_pluginsSet;

  private:
    static KateDocumentConfig *s_global;
    KateDocument *m_doc;
};

class KateViewConfig : public KateConfig
{
  private:
    friend class KateGlobal;

    /**
     * only used in KateGlobal for the static global fallback !!!
     */
    KateViewConfig ();

  public:
    /**
     * Construct a DocumentConfig
     */
    explicit KateViewConfig (KateView *view);

    /**
     * Cu DocumentConfig
     */
    ~KateViewConfig ();

    inline static KateViewConfig *global () { return s_global; }

    inline bool isGlobal () const { return (this == global()); }

  public:
    /**
     * Read config from object
     */
    void readConfig (const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig (KConfigGroup &config);

  protected:
    void updateConfig ();

  public:
    bool dynWordWrap () const;
    void setDynWordWrap (bool wrap);

    int dynWordWrapIndicators () const;
    void setDynWordWrapIndicators (int mode);

    int dynWordWrapAlignIndent () const;
    void setDynWordWrapAlignIndent (int indent);

    bool lineNumbers () const;
    void setLineNumbers (bool on);

    bool scrollBarMarks () const;
    void setScrollBarMarks (bool on);

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

    uint defaultMarkType () const;
    void setDefaultMarkType (uint type);

    bool persistentSelection () const;
    void setPersistentSelection (bool on);

    enum TextToSearch
    {
      Nowhere = 0,
      SelectionOnly = 1,
      SelectionWord = 2,
      WordOnly = 3,
      WordSelection = 4
    };

    int textToSearchMode () const;
    void setTextToSearchMode (int mode);

  private:
    bool m_dynWordWrap;
    int m_dynWordWrapIndicators;
    int m_dynWordWrapAlignIndent;
    bool m_lineNumbers;
    bool m_scrollBarMarks;
    bool m_iconBar;
    bool m_foldingBar;
    int m_bookmarkSort;
    int m_autoCenterLines;
    long m_searchFlags;
    uint m_defaultMarkType;
    bool m_persistentSelection;
    int m_textToSearchMode;

    bool m_dynWordWrapSet : 1;
    bool m_dynWordWrapIndicatorsSet : 1;
    bool m_dynWordWrapAlignIndentSet : 1;
    bool m_lineNumbersSet : 1;
    bool m_scrollBarMarksSet : 1;
    bool m_iconBarSet : 1;
    bool m_foldingBarSet : 1;
    bool m_bookmarkSortSet : 1;
    bool m_autoCenterLinesSet : 1;
    bool m_searchFlagsSet : 1;
    bool m_defaultMarkTypeSet : 1;
    bool m_persistentSelectionSet : 1;
    bool m_textToSearchModeSet : 1;

  private:
    static KateViewConfig *s_global;
    KateView *m_view;
};

class KateRendererConfig : public KateConfig
{
  private:
    friend class KateGlobal;

    /**
     * only used in KateGlobal for the static global fallback !!!
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

    inline static KateRendererConfig *global () { return s_global; }

    inline bool isGlobal () const { return (this == global()); }

  public:
    /**
     * Read config from object
     */
    void readConfig (const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig (KConfigGroup &config);

  protected:
    void updateConfig ();

  public:
    const QString &schema () const;
    void setSchema (const QString &schema);
    /**
     * Reload the schema from the schema manager.
     * For the global instance, have all other instances reload.
     * Used by the schema config page to apply changes.
     */
    void reloadSchema();

    const QFont& font() const;
    const QFontMetrics& fontMetrics() const;
    void setFont(const QFont &font);

    bool wordWrapMarker () const;
    void setWordWrapMarker (bool on);

    const QColor& backgroundColor() const;
    void setBackgroundColor (const QColor &col);

    const QColor& selectionColor() const;
    void setSelectionColor (const QColor &col);

    const QColor& highlightedLineColor() const;
    void setHighlightedLineColor (const QColor &col);

    const QColor& lineMarkerColor(KTextEditor::MarkInterface::MarkTypes type = KTextEditor::MarkInterface::markType01) const; // markType01 == Bookmark
    void setLineMarkerColor (const QColor &col, KTextEditor::MarkInterface::MarkTypes type = KTextEditor::MarkInterface::markType01);

    const QColor& highlightedBracketColor() const;
    void setHighlightedBracketColor (const QColor &col);

    const QColor& wordWrapMarkerColor() const;
    void setWordWrapMarkerColor (const QColor &col);

    const QColor& tabMarkerColor() const;
    void setTabMarkerColor (const QColor &col);

    const QColor& iconBarColor() const;
    void setIconBarColor (const QColor &col);

    // the line number color is used for the line numbers on the left bar and
    // for vertical separator lines and for code folding lines.
    const QColor& lineNumberColor() const;
    void setLineNumberColor (const QColor &col);

    bool showIndentationLines () const;
    void setShowIndentationLines (bool on);

    const QColor &templateBackgroundColor() const;
    const QColor &templateEditablePlaceholderColor() const;
    const QColor &templateFocusedEditablePlaceholderColor() const;
    const QColor &templateNotEditablePlaceholderColor() const;


  private:
    /**
     * Read the schema properties from the config file.
     */
    void setSchemaInternal(const QString &schema);

    QString m_schema;
    QFont m_font;
    QFontMetrics m_fontMetrics;
    bool m_wordWrapMarker;
    bool m_showIndentationLines;
    QColor m_backgroundColor;
    QColor m_selectionColor;
    QColor m_highlightedLineColor;
    QColor m_highlightedBracketColor;
    QColor m_wordWrapMarkerColor;
    QColor m_tabMarkerColor;
    QColor m_iconBarColor;
    QColor m_lineNumberColor;
    QVector<QColor> m_lineMarkerColor;

    QColor m_templateBackgroundColor;
    QColor m_templateEditablePlaceholderColor;
    QColor m_templateFocusedEditablePlaceholderColor;
    QColor m_templateNotEditablePlaceholderColor;


    bool m_schemaSet : 1;
    bool m_fontSet : 1;
    bool m_wordWrapMarkerSet : 1;
    bool m_showIndentationLinesSet : 1;
    bool m_backgroundColorSet : 1;
    bool m_selectionColorSet : 1;
    bool m_highlightedLineColorSet : 1;
    bool m_highlightedBracketColorSet : 1;
    bool m_wordWrapMarkerColorSet : 1;
    bool m_tabMarkerColorSet : 1;
    bool m_iconBarColorSet : 1;
    bool m_lineNumberColorSet : 1;
    bool m_templateColorsSet : 1;
    QBitArray m_lineMarkerColorSet;

  private:
    static KateRendererConfig *s_global;
    KateRenderer *m_renderer;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
