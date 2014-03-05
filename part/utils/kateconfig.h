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

#include "katepartprivate_export.h"

#include <ktexteditor/markinterface.h>
#include <kencodingprober.h>

#include <QtCore/QBitRef>
#include <QtGui/QColor>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtGui/QFontMetricsF>
#include <QStringListModel>

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

class KATEPART_TESTS_EXPORT KateGlobalConfig : public KateConfig
{
  private:
    friend class KateGlobal;

    /**
     * only used in KateGlobal for the static global fallback !!!
     */
    KateGlobalConfig ();

    /**
     * Destructor
     */
    ~KateGlobalConfig ();

  public:
    static KateGlobalConfig *global () { return s_global; }

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
    KEncodingProber::ProberType proberType () const
    {
      return m_proberType;
    }

    void setProberType (KEncodingProber::ProberType proberType);

    QTextCodec *fallbackCodec () const;
    const QString &fallbackEncoding () const;
    bool setFallbackEncoding (const QString &encoding);

  private:
    KEncodingProber::ProberType m_proberType;
    QString m_fallbackEncoding;

  private:
    static KateGlobalConfig *s_global;
};

class KATEPART_TESTS_EXPORT KateDocumentConfig : public KateConfig
{
  private:
    friend class KateGlobal;

    KateDocumentConfig ();

  public:
    KateDocumentConfig (const KConfigGroup &cg);

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

    int wordWrapAt () const;
    void setWordWrapAt (int col);

    bool pageUpDownMovesCursor () const;
    void setPageUpDownMovesCursor (bool on);

    void setKeepExtraSpaces (bool on);
    bool keepExtraSpaces () const;

    void setIndentPastedText (bool on);
    bool indentPastedText () const;

    void setBackspaceIndents (bool on);
    bool backspaceIndents () const;

    void setSmartHome (bool on);
    bool smartHome () const;

    void setShowTabs (bool on);
    bool showTabs() const;

    void setShowSpaces (bool on);
    bool showSpaces() const;

    void setReplaceTabsDyn (bool on);
    bool replaceTabsDyn() const;

    /**
     * Remove trailing spaces on save.
     * triState = 0: never remove trailing spaces
     * triState = 1: remove trailing spaces of modified lines (line modification system)
     * triState = 2: remove trailing spaces in entire document
     */
    void setRemoveSpaces (int triState);
    int removeSpaces () const;

    void setNewLineAtEof (bool on);
    bool newLineAtEof () const;

    void setOvr (bool on);
    bool ovr () const;

    void setTabIndents (bool on);
    bool tabIndentsEnabled () const;

    QTextCodec *codec () const;
    const QString &encoding () const;
    bool setEncoding (const QString &encoding);
    bool isSetEncoding () const;

    enum Eol
    {
      eolUnix = 0,
      eolDos = 1,
      eolMac = 2
    };

    int eol () const;
    QString eolString ();

    void setEol (int mode);

    bool bom () const;
    void setBom(bool bom);

    bool allowEolDetection () const;
    void setAllowEolDetection (bool on);

    bool allowSimpleMode () const;
    void setAllowSimpleMode (bool on);

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

    bool swapFileNoSync() const;
    void setSwapFileNoSync(bool on);

    /**
     * Should Kate Part search for dir wide config file
     * and if, how depth?
     * @return search depth (< 0 no search)
     */
    int searchDirConfigDepth () const;

    void setSearchDirConfigDepth (int depth);

    bool onTheFlySpellCheck() const;
    void setOnTheFlySpellCheck(bool on);

    int lineLengthLimit() const;
    void setLineLengthLimit(int limit);


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
    bool m_allowSimpleMode;
    int m_eol;
    bool m_bom;
    uint m_backupFlags;
    int m_searchDirConfigDepth;
    QString m_encoding;
    QString m_backupPrefix;
    QString m_backupSuffix;
    bool m_swapFileNoSync;
    bool m_onTheFlySpellCheck;
    int m_lineLengthLimit;

    bool m_tabWidthSet : 1;
    bool m_indentationWidthSet : 1;
    bool m_indentationModeSet : 1;
    bool m_wordWrapSet : 1;
    bool m_wordWrapAtSet : 1;
    bool m_pageUpDownMovesCursorSet : 1;

    bool m_keepExtraSpacesSet : 1;
    bool m_keepExtraSpaces : 1;
    bool m_indentPastedTextSet : 1;
    bool m_indentPastedText : 1;
    bool m_backspaceIndentsSet : 1;
    bool m_backspaceIndents : 1;
    bool m_smartHomeSet : 1;
    bool m_smartHome : 1;
    bool m_showTabsSet : 1;
    bool m_showTabs : 1;
    bool m_showSpacesSet : 1;
    bool m_showSpaces : 1;
    bool m_replaceTabsDynSet : 1;
    bool m_replaceTabsDyn : 1;
    bool m_removeSpacesSet : 1;
    uint m_removeSpaces : 2;
    bool m_newLineAtEofSet : 1;
    bool m_newLineAtEof : 1;
    bool m_overwiteModeSet : 1;
    bool m_overwiteMode : 1;
    bool m_tabIndentsSet : 1;
    bool m_tabIndents : 1;

    bool m_encodingSet : 1;
    bool m_eolSet : 1;
    bool m_bomSet :1;
    bool m_allowEolDetectionSet : 1;
    bool m_allowSimpleModeSet : 1;
    bool m_backupFlagsSet : 1;
    bool m_searchDirConfigDepthSet : 1;
    bool m_backupPrefixSet : 1;
    bool m_backupSuffixSet : 1;
    bool m_swapFileNoSyncSet : 1;
    bool m_onTheFlySpellCheckSet : 1;
    bool m_lineLengthLimitSet : 1;

  private:
    static KateDocumentConfig *s_global;
    KateDocument *m_doc;
};

class KATEPART_TESTS_EXPORT KateViewConfig : public KateConfig
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

    bool scrollBarMiniMap () const;
    void setScrollBarMiniMap (bool on);

    bool scrollBarMiniMapAll () const;
    void setScrollBarMiniMapAll (bool on);

    int scrollBarMiniMapWidth () const;
    void setScrollBarMiniMapWidth (int width);

    /* Whether to show scrollbars */
    enum ScrollbarMode {
      AlwaysOn = 0,
      ShowWhenNeeded,
      AlwaysOff
    };

    int showScrollbars () const;
    void setShowScrollbars (int mode);

    bool iconBar () const;
    void setIconBar (bool on);

    bool foldingBar () const;
    void setFoldingBar (bool on);

    bool lineModification() const;
    void setLineModification(bool on);

    int bookmarkSort () const;
    void setBookmarkSort (int mode);

    int autoCenterLines() const;
    void setAutoCenterLines (int lines);

    enum SearchFlags {
      IncMatchCase = 1 << 0,
      IncHighlightAll = 1 << 1,
      IncFromCursor = 1 << 2,
      PowerMatchCase = 1 << 3,
      PowerHighlightAll = 1 << 4,
      PowerFromCursor = 1 << 5,
      // PowerSelectionOnly = 1 << 6, Better not save to file // Sebastian
      PowerModePlainText = 1 << 7,
      PowerModeWholeWords = 1 << 8,
      PowerModeEscapeSequences = 1 << 9,
      PowerModeRegularExpression = 1 << 10,
      PowerUsePlaceholders = 1 << 11
    };

    long searchFlags () const;
    void setSearchFlags (long flags);

    int maxHistorySize() const;

    QStringListModel *patternHistoryModel();
    QStringListModel *replacementHistoryModel();

    uint defaultMarkType () const;
    void setDefaultMarkType (uint type);

    bool allowMarkMenu () const;
    void setAllowMarkMenu (bool allow);

    bool persistentSelection () const;
    void setPersistentSelection (bool on);

    bool viInputMode () const;
    void setViInputMode (bool on);

    bool viInputModeStealKeys () const;
    void setViInputModeStealKeys (bool on);

    bool viRelativeLineNumbers() const;
    void setViRelativeLineNumbers(bool on);

    bool viInputModeEmulateCommandBar() const;
    void setViInputModeEmulateCommandBar(bool on);

    // Do we still need the enum and related functions below?
    enum TextToSearch
    {
      Nowhere = 0,
      SelectionOnly = 1,
      SelectionWord = 2,
      WordOnly = 3,
      WordSelection = 4
    };

    bool automaticCompletionInvocation () const;
    void setAutomaticCompletionInvocation (bool on);

    bool wordCompletion () const;
    void setWordCompletion (bool on);

    bool keywordCompletion () const;
    void setKeywordCompletion (bool on);

    int wordCompletionMinimalWordLength () const;
    void setWordCompletionMinimalWordLength (int length);

    bool wordCompletionRemoveTail () const;
    void setWordCompletionRemoveTail (bool on);

    bool smartCopyCut() const;
    void setSmartCopyCut(bool on);

    bool scrollPastEnd() const;
    void setScrollPastEnd(bool on);

  private:
    bool m_dynWordWrap;
    int m_dynWordWrapIndicators;
    int m_dynWordWrapAlignIndent;
    bool m_lineNumbers;
    bool m_scrollBarMarks;
    bool m_scrollBarMiniMap;
    bool m_scrollBarMiniMapAll;
    int  m_scrollBarMiniMapWidth;
    int  m_showScrollbars;
    bool m_iconBar;
    bool m_foldingBar;
    bool m_lineModification;
    int m_bookmarkSort;
    int m_autoCenterLines;
    long m_searchFlags;
    int m_maxHistorySize;
    QStringListModel m_patternHistoryModel;
    QStringListModel m_replacementHistoryModel;
    uint m_defaultMarkType;
    bool m_persistentSelection;
    bool m_viInputMode;
    bool m_viInputModeStealKeys;
    bool m_viRelativeLineNumbers;
    bool m_viInputModeEmulateCommandBar;
    bool m_automaticCompletionInvocation;
    bool m_wordCompletion;
    bool m_keywordCompletion;
    int m_wordCompletionMinimalWordLength;
    bool m_wordCompletionRemoveTail;
    bool m_smartCopyCut;
    bool m_scrollPastEnd;

    bool m_dynWordWrapSet : 1;
    bool m_dynWordWrapIndicatorsSet : 1;
    bool m_dynWordWrapAlignIndentSet : 1;
    bool m_lineNumbersSet : 1;
    bool m_scrollBarMarksSet : 1;
    bool m_scrollBarMiniMapSet : 1;
    bool m_scrollBarMiniMapAllSet : 1;
    bool m_scrollBarMiniMapWidthSet : 1;
    bool m_showScrollbarsSet : 1;
    bool m_iconBarSet : 1;
    bool m_foldingBarSet : 1;
    bool m_lineModificationSet : 1;
    bool m_bookmarkSortSet : 1;
    bool m_autoCenterLinesSet : 1;
    bool m_searchFlagsSet : 1;
    bool m_defaultMarkTypeSet : 1;
    bool m_persistentSelectionSet : 1;
    bool m_viInputModeSet : 1;
    bool m_viInputModeStealKeysSet : 1;
    bool m_viRelativeLineNumbersSet : 1;
    bool m_viInputModeEmulateCommandBarSet : 1;
    bool m_automaticCompletionInvocationSet : 1;
    bool m_wordCompletionSet : 1;
    bool m_keywordCompletionSet : 1;
    bool m_wordCompletionMinimalWordLengthSet : 1;
    bool m_smartCopyCutSet : 1;
    bool m_scrollPastEndSet : 1;
    bool m_allowMarkMenu : 1;
    bool m_wordCompletionRemoveTailSet : 1;

  private:
    static KateViewConfig *s_global;
    KateView *m_view;
};

class KATEPART_TESTS_EXPORT KateRendererConfig : public KateConfig
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
    const QFontMetricsF& fontMetrics() const;
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

    const QColor& indentationLineColor() const;
    void setIndentationLineColor (const QColor &col);

    const QColor& iconBarColor() const;
    void setIconBarColor (const QColor &col);

    const QColor& foldingColor() const;
    void setFoldingColor (const QColor &col);

    // the line number color is used for the line numbers on the left bar
    const QColor& lineNumberColor() const;
    void setLineNumberColor (const QColor &col);

    // the color of the separator between line numbers and icon bar
    const QColor& separatorColor() const;
    void setSeparatorColor(const QColor &col);

    const QColor& spellingMistakeLineColor() const;
    void setSpellingMistakeLineColor (const QColor &col);

    bool showIndentationLines () const;
    void setShowIndentationLines (bool on);

    bool showWholeBracketExpression () const;
    void setShowWholeBracketExpression (bool on);

    bool animateBracketMatching () const;
    void setAnimateBracketMatching (bool on);

    const QColor &templateBackgroundColor() const;
    const QColor &templateEditablePlaceholderColor() const;
    const QColor &templateFocusedEditablePlaceholderColor() const;
    const QColor &templateNotEditablePlaceholderColor() const;

    const QColor& modifiedLineColor() const;
    void setModifiedLineColor(const QColor &col);

    const QColor& savedLineColor() const;
    void setSavedLineColor(const QColor &col);

    const QColor& searchHighlightColor() const;
    void setSearchHighlightColor(const QColor &col);

    const QColor& replaceHighlightColor() const;
    void setReplaceHighlightColor(const QColor &col);

  private:
    /**
     * Read the schema properties from the config file.
     */
    void setSchemaInternal(const QString &schema);

    QString m_schema;
    QFont m_font;
    QFontMetricsF m_fontMetrics;
    QColor m_backgroundColor;
    QColor m_selectionColor;
    QColor m_highlightedLineColor;
    QColor m_highlightedBracketColor;
    QColor m_wordWrapMarkerColor;
    QColor m_tabMarkerColor;
    QColor m_indentationLineColor;
    QColor m_iconBarColor;
    QColor m_foldingColor;
    QColor m_lineNumberColor;
    QColor m_separatorColor;
    QColor m_spellingMistakeLineColor;
    QVector<QColor> m_lineMarkerColor;

    QColor m_templateBackgroundColor;
    QColor m_templateEditablePlaceholderColor;
    QColor m_templateFocusedEditablePlaceholderColor;
    QColor m_templateNotEditablePlaceholderColor;

    QColor m_modifiedLineColor;
    QColor m_savedLineColor;
    QColor m_searchHighlightColor;
    QColor m_replaceHighlightColor;

    bool m_wordWrapMarker;
    bool m_showIndentationLines;
    bool m_showWholeBracketExpression;
    bool m_animateBracketMatching;

    bool m_schemaSet : 1;
    bool m_fontSet : 1;
    bool m_wordWrapMarkerSet : 1;
    bool m_showIndentationLinesSet : 1;
    bool m_showWholeBracketExpressionSet : 1;
    bool m_backgroundColorSet : 1;
    bool m_selectionColorSet : 1;
    bool m_highlightedLineColorSet : 1;
    bool m_highlightedBracketColorSet : 1;
    bool m_wordWrapMarkerColorSet : 1;
    bool m_tabMarkerColorSet : 1;
    bool m_indentationLineColorSet : 1;
    bool m_iconBarColorSet : 1;
    bool m_foldingColorSet : 1;
    bool m_lineNumberColorSet : 1;
    bool m_separatorColorSet : 1;
    bool m_spellingMistakeLineColorSet : 1;
    bool m_templateColorsSet : 1;
    bool m_modifiedLineColorSet : 1;
    bool m_savedLineColorSet : 1;
    bool m_searchHighlightColorSet : 1;
    bool m_replaceHighlightColorSet : 1;
    QBitArray m_lineMarkerColorSet;

  private:
    static KateRendererConfig *s_global;
    KateRenderer *m_renderer;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
