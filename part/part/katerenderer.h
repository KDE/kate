/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef __KATE_RENDERER_H__
#define __KATE_RENDERER_H__

#include "katecursor.h"
#include <ktexteditor/attribute.h>
#include "katetextline.h"
#include "katelinerange.h"

#include <qfont.h>
#include <qfontmetrics.h>
#include <QList>
#include <QTextLayout>

class KateDocument;
class KateView;
class KateRendererConfig;
namespace  KTextEditor { class Range; }

class KateLineLayout;
typedef KSharedPtr<KateLineLayout> KateLineLayoutPtr;

/**
 * Handles all of the work of rendering the text
 * (used for the views and printing)
 *
 **/
class KateRenderer
{
public:
    /**
     * Style of Caret (Insert or Replace mode)
     */
    enum caretStyles {
      Insert,
      Replace
    };

    /**
     * Constructor
     * @param doc document to render
     * @param view view which is output (0 for example for rendering to print)
     */
    KateRenderer(KateDocument* doc, KateView *view = 0);

    /**
     * Destructor
     */
    ~KateRenderer();

    /**
     * Returns the document to which this renderer is bound
     */
    KateDocument* doc() const { return m_doc; }

    /**
     * Returns the view to which this renderer is bound
     */
    KateView* view() const { return m_view; }

    /**
     * update the highlighting attributes
     * (for example after an hl change or after hl config changed)
     */
    void updateAttributes ();

    /**
     * Determine whether the caret (text cursor) will be drawn.
     * @return should it be drawn?
     */
    inline bool drawCaret() const { return m_drawCaret; }

    /**
     * Set whether the caret (text cursor) will be drawn.
     * @param drawCaret should caret be drawn?
     */
    void setDrawCaret(bool drawCaret);

    /**
     * The style of the caret (text cursor) to be painted.
     * @return caretStyle
     */
    inline KateRenderer::caretStyles caretStyle() const { return m_caretStyle; }

    /**
     * Set the style of caret to be painted.
     * @param style style to set
     */
    void setCaretStyle(KateRenderer::caretStyles style);

    /**
     * @returns whether tabs should be shown (ie. a small mark
     * drawn to identify a tab)
     * @return tabs should be shown
     */
    inline bool showTabs() const { return m_showTabs; }

    /**
     * Set whether a mark should be painted to help identifying tabs.
     * @param showTabs show the tabs?
     */
    void setShowTabs(bool showTabs);

    /**
     * Sets the width of the tab. Helps performance.
     * @param tabWidth new tab width
     */
    void setTabWidth(int tabWidth);

    /**
     * @returns whether indent lines should be shown 
     * @return indent lines should be shown
     */
    bool showIndentLines() const;

    /**
     * Set whether a guide should be painted to help identifying indent lines.
     * @param showLines show the indent lines?
     */
    void setShowIndentLines(bool showLines);
    
    /**
     * Sets the width of the tab. Helps performance.
     * @param indentWidth new indent width
     */
    void setIndentWidth(int indentWidth);
    
    /**
     * Show the view's selection?
     * @return show sels?
     */
    inline bool showSelections() const { return m_showSelections; }

    /**
     * Set whether the view's selections should be shown.
     * The default is true.
     * @param showSelections show the selections?
     */
    void setShowSelections(bool showSelections);

    /**
     * Change to a different font (soon to be font set?)
     */
    void increaseFontSizes();
    void decreaseFontSizes();
    const QFont* currentFont() const;
    const QFontMetrics* currentFontMetrics() const;

    /**
     * @return whether the renderer is configured to paint in a
     * printer-friendly fashion.
     */
    bool isPrinterFriendly() const;

    /**
     * Configure this renderer to paint in a printer-friendly fashion.
     *
     * Sets the other options appropriately if true.
     */
    void setPrinterFriendly(bool printerFriendly);

    /**
     * Text width & height calculation functions...
     */
    void layoutLine(KateLineLayoutPtr line, int maxwidth = -1, bool cacheLayout = false) const;
    QVector<QTextLayout::FormatRange> selectionDecorationsForLine(KateLineLayoutPtr range) const;
    QList<QTextLayout::FormatRange> decorationsForLine(KateLineLayoutPtr range) const;

    // Width calculators
    uint spaceWidth() const;
    uint textWidth(const KateTextLine::Ptr &, int cursorCol) KDE_DEPRECATED;
    uint textWidth(const KateTextLine::Ptr &textLine, uint startcol, uint maxwidth, bool *needWrap, int *endX = 0)  KDE_DEPRECATED;
    uint textWidth(const KTextEditor::Cursor& cursor)  KDE_DEPRECATED;

    /**
     * Returns the x position of cursor \p col on the line \p range. If \p doLayout
     * is false, \p col must be on the line laid out in \p range, else the function
     * will return -1.  If \p doLayout is true, the text will be laid out until the
     * answer is found.
     */
    int cursorToX(const KateTextLayout& range, int col, int maxwidth = -1) const;
    /// \overload
    int cursorToX(const KateTextLayout& range, const KTextEditor::Cursor& pos, int maxwidth = -1) const;

    /**
     * Returns the real cursor which is occupied by the specified x value, or that closest to it.
     * If \p returnPastLine is true, the column will be extrapolated out with the assumption
     * that the extra characters are spaces.
     */
    KTextEditor::Cursor xToCursor(const KateTextLayout& range, int x, bool returnPastLine = false) const;

    // Font height
    uint fontHeight();

    // Document height
    uint documentHeight();

    // Selection boundaries
    bool getSelectionBounds(int line, int lineLength, int &start, int &end) const;

    /**
     * This is the ultimate function to perform painting of a text line.
     *
     * The text line is painted from the upper limit of (0,0).  To move that,
     * apply a transform to your painter.
     *
     * @param paint           painter to use
     * @param range           layout to use in painting this line
     * @param xStart          starting width in pixels.
     * @param xEnd            ending width in pixels.
     * @param cursor          position of the caret, if placed on the current line.
     */
    void paintTextLine(QPainter& paint, KateLineLayoutPtr range, int xStart, int xEnd, const KTextEditor::Cursor* cursor = 0L);

    /**
     * Paint the background of a line
     *
     * Split off from the main @ref paintTextLine method to make it smaller. As it's being
     * called only once per line it shouldn't noticably affect performance and it
     * helps readability a LOT.
     *
     * @param paint           painter to use
     * @param layout          layout to use in painting this line
     * @param currentViewLine if one of the view lines is the current line, set
     *                        this to the index; otherwise -1.
     * @param xStart          starting width in pixels.
     * @param xEnd            ending width in pixels.
     */
    void paintTextLineBackground(QPainter& paint, KateLineLayoutPtr layout, int currentViewLine, int xStart, int xEnd);

    /**
     * This takes an in index, and returns all the attributes for it.
     * For example, if you have a ktextline, and want the KTextEditor::Attribute
     * for a given position, do:
     *
     *   attribute(myktextline->attribute(position));
     */
    KTextEditor::Attribute* attribute(uint pos) const;
    KTextEditor::Attribute* specificAttribute(int context) const;

  private:
    /**
     * Paint a whitespace marker on position (x, y).
     *
     * Currently only used by the tabs, but it will also be used for highlighting trailing whitespace
     */
    void paintWhitespaceMarker(QPainter &paint, uint x, uint y);

    /** Paint a SciTE-like indent marker. */
    void paintIndentMarker(QPainter &paint, uint x, uint y);

    KateDocument* m_doc;
    KateView *m_view;

    // cache of config values
    int m_tabWidth;
    int m_indentWidth;
    uint m_schema;

    // some internal flags
    KateRenderer::caretStyles m_caretStyle;
    bool m_drawCaret;
    bool m_showSelections;
    bool m_showTabs;
    bool m_printerFriendly;

    QVector<KTextEditor::Attribute> *m_attributes;

  /**
   * Configuration
   */
  public:
    inline KateRendererConfig *config () const { return m_config; };

    void updateConfig ();

  private:
    KateRendererConfig *m_config;
};

#endif
