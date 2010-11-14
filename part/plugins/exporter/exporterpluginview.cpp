/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 * Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 * Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 * Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 * Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "exporterpluginview.h"

#include "exporterplugin.h"
#include "abstractexporter.h"
#include "htmlexporter.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/highlightinterface.h>

#include <kactioncollection.h>
#include <kaction.h>
#include <kfiledialog.h>
#include <ktemporaryfile.h>
#include <ksavefile.h>
#include <kio/netaccess.h>

#include <QtCore/QMimeData>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QTextCodec>

#include <kdebug.h>

ExporterPluginView::ExporterPluginView(KTextEditor::View* view)
  : QObject(view), KXMLGUIClient(view), m_view(view)
{
  setComponentData( ExporterPluginFactory::componentData() );
  setXMLFile("ktexteditor_exporterui.rc");

  m_copyAction = actionCollection()->addAction("edit_copy_html", this, SLOT(exportToClipboard()));
  m_copyAction->setIcon(KIcon("edit-copy"));
  m_copyAction->setText(i18n("Copy as &HTML"));
  m_copyAction->setWhatsThis(i18n("Use this command to copy the currently selected text as HTML to the system clipboard."));
  m_copyAction->setEnabled(m_view->selection());

  m_fileExportAction = actionCollection()->addAction("file_export_html", this, SLOT(exportToFile()));
  m_fileExportAction->setText(i18n("E&xport as HTML..."));
  m_fileExportAction->setWhatsThis(i18n("This command allows you to export the current document"
                      " with all highlighting information into a HTML document."));

  connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)),
          this, SLOT(updateSelectionAction(KTextEditor::View*)));
}

ExporterPluginView::~ExporterPluginView()
{
}

void ExporterPluginView::updateSelectionAction(KTextEditor::View* view)
{
  Q_ASSERT(view == m_view); Q_UNUSED(view)
  m_copyAction->setEnabled(m_view->selection());
}

void ExporterPluginView::exportToClipboard()
{
  if (!m_view->selection()) {
    return;
  }

  QMimeData *data = new QMimeData();

  QString s;
  QTextStream output( &s, QIODevice::WriteOnly );
  exportData(true, output);

  data->setHtml(s);
  data->setText(s);

  QApplication::clipboard()->setMimeData(data);
}

void ExporterPluginView::exportToFile()
{
  KUrl url = KFileDialog::getSaveUrl(m_view->document()->documentName(), "text/html",
                                     m_view, i18n("Export File as HTML"));

  if ( url.isEmpty() ) {
    return;
  }

  QString filename;

  if ( url.isLocalFile() ) {
    filename = url.toLocalFile();
  } else {
    ///TODO: cleanup! don't let the temp files lay around
    KTemporaryFile tmp; // ### only used for network export
    tmp.setAutoRemove(false);
    tmp.open();
    filename = tmp.fileName();
  }

  KSaveFile savefile(filename);
  if (savefile.open()) {
    QTextStream outputStream ( &savefile );

    exportData(false, outputStream);

    savefile.finalize(); //check error?
  }
//     else
//       {/*ERROR*/}

  if ( !url.isLocalFile() ) {
    KIO::NetAccess::upload( filename, url, 0 );
  }
}

void ExporterPluginView::exportData(const bool useSelection, QTextStream &output)
{
  const KTextEditor::Range range = useSelection ? m_view->selectionRange() : m_view->document()->documentRange();
  const bool blockwise = useSelection ? m_view->blockSelection() : false;

  if ( (blockwise || range.onSingleLine()) && (range.start().column() > range.end().column() ) ) {
    return;
  }

  //outputStream.setEncoding(QTextStream::UnicodeUTF8);
  output.setCodec(QTextCodec::codecForName("UTF-8"));

  ///TODO: add more exporters
  AbstractExporter* exporter;

  exporter = new HTMLExporter(m_view, output, !useSelection);

  KTextEditor::HighlightInterface* hiface = qobject_cast<KTextEditor::HighlightInterface*>(m_view->document());

  const KTextEditor::Attribute::Ptr noAttrib(0);

  for (int i = range.start().line(); (i <= range.end().line()) && (i < m_view->document()->lines()); ++i)
  {
    const QString &line = m_view->document()->line(i);

    QList<KTextEditor::HighlightInterface::AttributeBlock> attribs;
    if ( hiface ) {
      attribs = hiface->lineAttributes(i);
    }

    int lineStart = 0;
    int remainingChars = line.length();
    if ( blockwise || range.onSingleLine() ) {
      lineStart = range.start().column();
      remainingChars = range.columnWidth();
    } else if ( i == range.start().line() ) {
      lineStart = range.start().column();
    } else if ( i == range.end().line() ) {
      remainingChars = range.end().column();
    }

    int handledUntil = lineStart;

    foreach ( const KTextEditor::HighlightInterface::AttributeBlock& block, attribs ) {
      // honor (block-) selections
      if ( block.start + block.length <= lineStart ) {
        continue;
      } else if ( block.start >= lineStart + remainingChars ) {
        break;
      }
      int start = qMax(block.start, lineStart);
      if ( start > handledUntil ) {
        exporter->exportText( line.mid( handledUntil, start - handledUntil ), noAttrib );
      }
      int length = qMin(block.length, remainingChars);
      exporter->exportText( line.mid( start, length ), block.attribute);
      handledUntil = start + length;
    }

    if ( handledUntil < lineStart + remainingChars ) {
      exporter->exportText( line.mid( handledUntil, remainingChars ), noAttrib );
    }

    exporter->closeLine(i == range.end().line());
  }

  delete exporter;

  output.flush();
}

#include "exporterpluginview.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
