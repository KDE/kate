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
#include "katefont.h"
#include "katetextline.h"

class KateDocument;
class KateView;
class LineRange;

/**
 * Handles all of the work in directly rendering Kate's view.
 *
 * TODO: move colours into this
 **/
class KateRenderer
{
public:
    enum caretStyles {
      Insert,
      Replace
    };

    // use different fonts for screen and printing
    enum WhichFont
    {
      ViewFont = 1,
      PrintFont = 2
    };

    KateRenderer(KateDocument* doc);
    ~KateRenderer();

    /**
     * The renderer supports different configurations depending on the view defined.
     *
     * Additionally this is used by the highlighting interface to provide per-view highlighting capabilities.
     */
    KateView* currentView() const;

    /**
     * @copydoc currentView()
     *
     * Switching the view saves the current settings, and restores the settings of the new current view.
     */
    void setView(KateView* view);

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
     * Returns the size of a tab (in chars)
     */
    static int tabWidth();

    /**
     * Sets the size of a tab in chars.
     */
    static void setTabWidth(int m_tabWidth);

    /**
     * @returns whether tabs should be shown (ie. a small mark drawn to identify a tab)
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
    const QFont& currentFont() const;
    const QFontMetrics& currentFontMetrics() const;

    static const FontStruct& getFontStruct(int whichFont);
    static void setFont(int whichFont, QFont font);
    static const QFont& getFont(int whichFont);
    static const QFontMetrics& getFontMetrics(int whichFont);

    /**
     * @returns whether the renderer is configured to paint in a printer-friendly fashion.
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
    KateView* m_view;
    QMap<KateView*, class KateRendererSettings*> m_settings;
    KateRendererSettings* m_currentSettings;
};

#endif
