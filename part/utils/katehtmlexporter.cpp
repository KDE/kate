/* This file is part of the KDE libraries
   Copyright (C) 2009 Bernhard Beschow <bbeschow at cs.tu-berlin.de>

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

#include "katehtmlexporter.h"
#include "katehtmlexporter.moc"

#include "katerenderer.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katetextline.h"
#include "kateview.h"

#include <kio/netaccess.h>

#include <kapplication.h>
#include <klocale.h>
#include <kaction.h>
#include <kfiledialog.h>
#include <ktemporaryfile.h>
#include <ksavefile.h>
#include <kactioncollection.h>

#include <QtGui/QClipboard>
#include <QtGui/QTextDocument>
#include <QtCore/QTextCodec>

KateHTMLExporter::KateHTMLExporter( KateView *view )
    : QObject( view )
    , KXMLGUIClient( view )
    , m_view( view )
{
  setComponentData(KateGlobal::self()->componentData());
  setXMLFile("katepart_htmlexporterui.rc");

  KActionCollection *ac = actionCollection ();

  m_copyHTML = ac->addAction("edit_copy_html", this, SLOT(copyHTML()));
  m_copyHTML->setIcon(KIcon("edit-copy"));
  m_copyHTML->setText(i18n("Copy as &HTML"));
  m_copyHTML->setWhatsThis(i18n("Use this command to copy the currently selected text as HTML to the system clipboard."));

  m_exportHTML = ac->addAction("file_export_html", this, SLOT(exportAsHTML()));
  m_exportHTML->setText(i18n("E&xport as HTML..."));
  m_exportHTML->setWhatsThis(i18n("This command allows you to export the current document"
                      " with all highlighting information into a HTML document."));

  connect(view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(slotUpdateActions()));

  slotUpdateActions();
}

KateHTMLExporter::~KateHTMLExporter()
{}

void KateHTMLExporter::slotUpdateActions ()
{
  m_copyHTML->setEnabled (m_view->selection());
}

void KateHTMLExporter::copyHTML()
{
  if (!m_view->selection())
    return;

  QMimeData *data = new QMimeData();
  data->setText(m_view->selectionText());
  data->setHtml(selectionAsHtml());
  QApplication::clipboard()->setMimeData(data);
}

QString KateHTMLExporter::selectionAsHtml()
{
  KTextEditor::Range range = m_view->selectionRange();
  const bool blockwise = m_view->blockSelectionMode();

  if (blockwise)
    KateView::blockFix(range);

  return textAsHtml(range, blockwise);
}

QString KateHTMLExporter::textAsHtml ( const KTextEditor::Range &range, bool blockwise)
{
  kDebug(13020) << "textAsHtml";

  QString s;
  QTextStream ts( &s, QIODevice::WriteOnly );
  ts.setCodec(QTextCodec::codecForName("UTF-8"));

  ts << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
  ts << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
  ts << "<head>" << endl;
  ts << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
  ts << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
  ts << "</head>" << endl;

  ts << "<body>" << endl;
  textAsHtmlStream(range, blockwise, ts);

  ts << "</body>" << endl;
  ts << "</html>" << endl;

  kDebug(13020) << "html is: " << s;

  return s;
}

void KateHTMLExporter::textAsHtmlStream ( const KTextEditor::Range& range, bool blockwise, QTextStream &ts)
{
  if ( (blockwise || range.onSingleLine()) && (range.start().column() > range.end().column() ) )
    return;

  if (range.onSingleLine())
  {
    KateTextLine::Ptr textLine = document()->kateTextLine(range.start().line());
    if ( !textLine )
      return;

    ts << "<pre>" << endl;

    lineAsHTML(*textLine, range.start().column(), range.columnWidth(), ts);
  }
  else
  {
    ts << "<pre>" << endl;

    for (int i = range.start().line(); (i <= range.end().line()) && (i < document()->lines()); ++i)
    {
      KateTextLine::Ptr textLine = document()->kateTextLine(i);

      if ( !blockwise )
      {
        if (i == range.start().line())
          lineAsHTML(*textLine, range.start().column(), textLine->length() - range.start().column(), ts);
        else if (i == range.end().line())
          lineAsHTML(*textLine, 0, range.end().column(), ts);
        else
          lineAsHTML(*textLine, 0, textLine->length(), ts);
      }
      else
      {
        lineAsHTML(*textLine, range.start().column(), range.columnWidth(), ts);
      }

      if ( i < range.end().line() )
        ts << "\n";    //we are inside a <pre>, so a \n is a new line
    }
  }
  ts << "</pre>";
}

void KateHTMLExporter::lineAsHTML (const KateTextLine &line, const int startCol, const int length, QTextStream &outputStream)
{
  const QColor blackColor(Qt::black);

  // some variables :
  bool previousCharacterWasBold = false;
  bool previousCharacterWasItalic = false;
  // when entering a new color, we'll close all the <b> & <i> tags,
  // for HTML compliancy. that means right after that font tag, we'll
  // need to reinitialize the <b> and <i> tags.
  bool needToReinitializeTags = false;
  QColor previousCharacterColor(blackColor); // default color of HTML characters is black


  // for each character of the line : (curPos is the position in the line)
  for (int curPos=startCol;curPos<(length+startCol);curPos++)
    {
      KTextEditor::Attribute::Ptr charAttributes = renderer()->attribute(line.attribute(curPos));

      //ASSERT(charAttributes != NULL);
      // let's give the color for that character :
      if ( (charAttributes->foreground() != previousCharacterColor))
      {  // the new character has a different color :
        // if we were in a bold or italic section, close it
        if (previousCharacterWasBold)
          outputStream << "</b>";
        if (previousCharacterWasItalic)
          outputStream << "</i>";

        // close the previous font tag :
  if(previousCharacterColor != blackColor)
          outputStream << "</span>";
        // let's read that color :
        int red, green, blue;
        // getting the red, green, blue values of the color :
        charAttributes->foreground().color().getRgb(&red, &green, &blue);
  if(!(red == 0 && green == 0 && blue == 0)) {
          outputStream << "<span style='color: #"
              << ( (red < 0x10)?"0":"")  // need to put 0f, NOT f for instance. don't touch 1f.
              << QString::number(red, 16) // html wants the hex value here (hence the 16)
              << ( (green < 0x10)?"0":"")
              << QString::number(green, 16)
              << ( (blue < 0x10)?"0":"")
              << QString::number(blue, 16)
              << "'>";
  }
        // we need to reinitialize the bold/italic status, since we closed all the tags
        needToReinitializeTags = true;
      }
      // bold status :
      if ( (needToReinitializeTags && charAttributes->fontBold()) ||
          (!previousCharacterWasBold && charAttributes->fontBold()) )
        // we enter a bold section
        outputStream << "<b>";
      if ( !needToReinitializeTags && (previousCharacterWasBold && !charAttributes->fontBold()) )
        // we leave a bold section
        outputStream << "</b>";

      // italic status :
      if ( (needToReinitializeTags && charAttributes->fontItalic()) ||
           (!previousCharacterWasItalic && charAttributes->fontItalic()) )
        // we enter an italic section
        outputStream << "<i>";
      if ( !needToReinitializeTags && (previousCharacterWasItalic && !charAttributes->fontItalic()) )
        // we leave an italic section
        outputStream << "</i>";

      // write the actual character :
      outputStream << Qt::escape(QString(line.at(curPos)));

      // save status for the next character :
      previousCharacterWasItalic = charAttributes->fontItalic();
      previousCharacterWasBold = charAttributes->fontBold();
      previousCharacterColor = charAttributes->foreground().color();
      needToReinitializeTags = false;
    }
  // Be good citizens and close our tags
  if (previousCharacterWasBold)
    outputStream << "</b>";
  if (previousCharacterWasItalic)
    outputStream << "</i>";

  if(previousCharacterColor != blackColor)
    outputStream << "</span>";
}

KateDocument *KateHTMLExporter::document()
{
  return m_view->doc();
}

KateRenderer *KateHTMLExporter::renderer()
{
  return m_view->renderer();
}

void KateHTMLExporter::exportAsHTML ()
{
  KUrl url = KFileDialog::getSaveUrl(document()->documentName(), "text/html",
                                     m_view, i18n("Export File as HTML"));

  if ( url.isEmpty() )
    return;

  QString filename;

  if ( url.isLocalFile() )
    filename = url.toLocalFile();
  else {
    KTemporaryFile tmp; // ### only used for network export
    tmp.setAutoRemove(false);
    tmp.open();
    filename = tmp.fileName();
  }

  KSaveFile savefile(filename);
  if (savefile.open())
  {
    QTextStream outputStream ( &savefile );

    //outputStream.setEncoding(QTextStream::UnicodeUTF8);
    outputStream.setCodec(QTextCodec::codecForName("UTF-8"));
    // let's write the HTML header :
    outputStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    outputStream << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
    outputStream << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
    outputStream << "<head>" << endl;
    outputStream << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
    outputStream << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
    // for the title, we write the name of the file (/usr/local/emmanuel/myfile.cpp -> myfile.cpp)
    outputStream << "<title>" << document()->documentName () << "</title>" << endl;
    outputStream << "</head>" << endl;
    outputStream << "<body>" << endl;

    textAsHtmlStream(document()->documentRange(), false, outputStream);

    outputStream << "</body>" << endl;
    outputStream << "</html>" << endl;
    outputStream.flush();

    savefile.finalize(); //check error?
  }
//     else
//       {/*ERROR*/}

  if ( url.isLocalFile() )
      return;

  KIO::NetAccess::upload( filename, url, 0 );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
