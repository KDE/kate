/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
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

#ifndef kate_htmlexporter_h
#define kate_htmlexporter_h

#include <QtCore/QObject>

#include <kxmlguiclient.h>

namespace KTextEditor {
  class Range;
}

class KateDocument;
class KateRenderer;
class KateTextLine;
class KateView;

class QAction;
class QTextStream;

class KateHTMLExporter : public QObject, public KXMLGUIClient
{
    Q_OBJECT

  public:
    KateHTMLExporter (KateView *view);
    ~KateHTMLExporter ();

  public Q_SLOTS:
    /**
     * internal use, copy text as HTML to clipboard
     */
    void copyHTML();

    void exportAsHTML ();

  private:
    QString selectionAsHtml ();
    QString textAsHtml (const KTextEditor::Range &range, bool blockwise);
    void textAsHtmlStream (const KTextEditor::Range& range, bool blockwise, QTextStream &ts);

    /**
     * Gets a substring in valid-xml html.
     * Example:  "<b>const</b> b = <i>34</i>"
     * It won't contain <p> or <body> or <html> or anything like that.
     *
     * @param startCol start column of substring
     * @param length length of substring
     * @param renderer The katerenderer.  This will have the schema
     *                 information that describes how to render the
     *                 attributes.
     * @param outputStream A stream to write the html to
     */
    void lineAsHTML (const KateTextLine &line, const int startCol, const int length, QTextStream &outputStream);

    KateDocument *document();
    KateRenderer *renderer();

  private Q_SLOTS:
    /**
     * used to update actions after selection changed
     */
    void slotUpdateActions();

  private:
    KateView *const m_view;
    QAction *m_exportHTML;
    QAction *m_copyHTML;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
