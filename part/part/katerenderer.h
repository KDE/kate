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

#ifndef KATERENDERER_H
#define KATERENDERER_H

#include "katecursor.h"
#include "katetextline.h"

#include <qfont.h>
#include <qfontmetrics.h>

class KateDocument;
class KateView;
class LineRange;
class KateRendererConfig;

/**
 * Handles all of the work in directly rendering Kate's view.
 *
 **/
class KateRenderer
{
public:
    enum caretStyles {
      Insert,
      Replace
    };

    KateRenderer(KateDocument* doc, KateView *view = 0);
    ~KateRenderer();

    /**
     * Determine whether the caret (text cursor) will be drawn.
     */
    bool drawCaret() const;

    /**
     * Set whether the caret (text cursor) will be drawn.
     */
    void setDrawCaret(bool drawCaret);

    /**
     * The style of the caret (text cursor) to be painted.
     */
    bool caretStyle() const;

    /**
     * Set the style of caret to be painted.
     */
    void setCaretStyle(int style);

    /**
     * @returns whether tabs should be shown (ie. a small mark
     * drawn to identify a tab)
     */
    bool showTabs() const;

    /**
     * Set whether a mark should be painted to help identifying tabs.
     */
    void setShowTabs(bool showTabs);

    /**
     * Show the view's selection?
     */
    bool showSelections() const;

    /**
     * Set whether the view's selections should be shown.
     * The default is true.
     */
    void setShowSelections(bool showSelections);

    /**
     * Index of the current font (or soon to be font set?)
     */
    int font() const;

    /**
     * Change to a different font (soon to be font set?)
     */
    void setFont(int whichFont);
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
    uint textWidth(const TextLine::Ptr &, int cursorCol);
    uint textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, bool *needWrap, int *endX = 0);
    uint textWidth(const KateTextCursor &cursor);

    // Cursor constrainer
    uint textWidth(KateTextCursor &cursor, int xPos, uint startCol = 0);

    // Column calculators
    uint textPos(uint line, int xPos, uint startCol = 0);
    uint textPos(const TextLine::Ptr &, int xPos, uint startCol = 0);

    // Font height
    uint fontHeight();

    // Document height
    uint documentHeight();

    // Selection boundaries
    bool selectBounds(uint line, uint &start, uint &end, uint lineLength);

    /**
     * This is the ultimate function to perform painting of a text line.
     * (supports startcol/endcol, startx/endx)
     *
     * The text line is painted from the upper limit of (0,0).  To move that,
     * apply a transform to your painter.
     */
    void paintTextLine(QPainter& paint, const LineRange* range, int xStart, int xEnd, const KateTextCursor* cursor = 0L, const KateTextRange* bracketmark = 0L);

  private:
    KateDocument* m_doc;
    KateView *m_view;
    class KateRendererSettings* m_currentSettings;

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
