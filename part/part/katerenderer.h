/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_RENDERER_H__
#define __KATE_RENDERER_H__

#include "katecursor.h"
#include "kateattribute.h"
#include "katetextline.h"

#include <qfont.h>
#include <qfontmetrics.h>

class KateDocument;
class KateView;
class KateLineRange;
class KateRendererConfig;

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
     * update the highlighting attributes
     * (for example after an hl change or after hl config changed)
     */
    void updateAttributes ();

    /**
     * Determine whether the caret (text cursor) will be drawn.
     * @param should it be drawn?
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
    const QFont* currentFont();
    const QFontMetrics* currentFontMetrics();

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

    // Width calculators
    uint spaceWidth();
    uint textWidth(const KateTextLine::Ptr &, int cursorCol);
    uint textWidth(const KateTextLine::Ptr &textLine, uint startcol, uint maxwidth, bool *needWrap, int *endX = 0);
    uint textWidth(const KateTextCursor &cursor);

    // Cursor constrainer
    uint textWidth(KateTextCursor &cursor, int xPos, uint startCol = 0);

    // Column calculators
    /**
     * @return the index of the character at the horixontal position @p xpos
     * in @p line.
     *
     * If @p nearest is true, the character starting nearest to
     * @p xPos is returned. If @p nearest is false, the index of the character
     * containing @p xPos is returned.
     **/
    uint textPos(uint line, int xPos, uint startCol = 0, bool nearest=true);
    /**
     * @overload
     */
    uint textPos(const KateTextLine::Ptr &, int xPos, uint startCol = 0, bool nearest=true);

    // Font height
    uint fontHeight();

    // Document height
    uint documentHeight();

    // Selection boundaries
    bool getSelectionBounds(uint line, uint lineLength, uint &start, uint &end);

    /**
     * This is the ultimate function to perform painting of a text line.
     * (supports startcol/endcol, startx/endx)
     *
     * The text line is painted from the upper limit of (0,0).  To move that,
     * apply a transform to your painter.
     */
    void paintTextLine(QPainter& paint, const KateLineRange* range, int xStart, int xEnd, const KateTextCursor* cursor = 0L, const KateBracketRange* bracketmark = 0L);

    /**
     * Paint the background of a line
     *
     * Split off from the main @ref paintTextLine method to make it smaller. As it's being
     * called only once per line it shouldn't noticably affect performance and it
     * helps readability a LOT.
     *
     * @return whether the selection has been painted or not
     */
    bool paintTextLineBackground(QPainter& paint, int line, bool isCurrentLine, int xStart, int xEnd);

    /**
     * This takes an in index, and returns all the attributes for it.
     * For example, if you have a ktextline, and want the KateAttribute
     * for a given position, do:
     *
     *   attribute(myktextline->attribute(position));
     */
    KateAttribute* attribute(uint pos);

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

    QMemArray<KateAttribute> *m_attributes;

  /**
   * Configuration
   */
  public:
    inline KateRendererConfig *config () { return m_config; };

    void updateConfig ();

  private:
    KateRendererConfig *m_config;
};

#endif
