/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2002-2010 Anders Lund <anders@alweb.dk>
 *
 *  Rewritten based on code of Copyright (c) 2002 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprinter.h"

#include "kateconfig.h"
#include "katedocument.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KDEPrintDialog>

#include <QApplication>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>

#include "printconfigwidgets.h"
#include "printpainter.h"

using namespace KatePrinter;

//BEGIN KatePrinterPrivate
class KatePrinterPrivate : public QObject
{
  Q_OBJECT
  public:
    KatePrinterPrivate(KateDocument *doc);
    ~KatePrinterPrivate();

    bool print(QPrinter *printer);

  public Q_SLOTS:
    void paint(QPrinter *printer);

  private:
    KateDocument *m_doc;
    PrintPainter *m_painter;
};

KatePrinterPrivate::KatePrinterPrivate(KateDocument* doc)
  : QObject()
  , m_doc(doc)
  , m_painter(new PrintPainter(doc))
{
}

KatePrinterPrivate::~KatePrinterPrivate()
{
  delete m_painter;
}

bool KatePrinterPrivate::print(QPrinter *printer)
{
  // docname is now always there, including the right Untitled name
  printer->setDocName(m_doc->documentName());

  KatePrintTextSettings *kpts = new KatePrintTextSettings;
  KatePrintHeaderFooter *kphf = new KatePrintHeaderFooter;
  KatePrintLayout *kpl = new KatePrintLayout;

  QList<QWidget*> tabs;
  tabs << kpts;
  tabs << kphf;
  tabs << kpl;

  QWidget *parentWidget = m_doc->widget();

  if (!parentWidget) {
    parentWidget = QApplication::activeWindow();
  }

  QScopedPointer<QPrintDialog> printDialog(KdePrint::createPrintDialog(printer, KdePrint::SystemSelectsPages, tabs, parentWidget));

  if (m_doc->activeView()->selection()) {
    printer->setPrintRange(QPrinter::Selection);
    printDialog->setOption(QAbstractPrintDialog::PrintSelection, true);
  }

  printDialog->setOption(QAbstractPrintDialog::PrintPageRange, true);

  if (!printDialog->exec())
    return false;

  // configure the painter
  m_painter->setPrintGuide(kpts->printGuide());
  m_painter->setPrintLineNumbers(kpts->printLineNumbers());

  m_painter->setColorScheme(kpl->colorScheme());
  m_painter->setUseBackground(kpl->useBackground());
  m_painter->setUseBox(kpl->useBox());
  m_painter->setBoxMargin(kpl->boxMargin());
  m_painter->setBoxWidth(kpl->boxWidth());
  m_painter->setBoxColor(kpl->boxColor());

  m_painter->setHeadersFont(kphf->font());

  m_painter->setUseHeader(kphf->useHeader());
  m_painter->setHeaderBackground(kphf->headerBackground());
  m_painter->setHeaderForeground(kphf->headerForeground());
  m_painter->setUseHeaderBackground(kphf->useHeaderBackground());
  m_painter->setHeaderFormat(kphf->headerFormat());

  m_painter->setUseFooter(kphf->useFooter());
  m_painter->setFooterBackground(kphf->footerBackground());
  m_painter->setFooterForeground(kphf->footerForeground());
  m_painter->setUseFooterBackground(kphf->useFooterBackground());
  m_painter->setFooterFormat(kphf->footerFormat());

  m_painter->paint(printer);
  return true;
}

void KatePrinterPrivate::paint(QPrinter *printer)
{
  m_painter->paint(printer);
}
//END KatePrinterPrivate

//BEGIN KatePrinter

bool KatePrinter::print(KateDocument *doc)
{
  QPrinter printer;
  KatePrinterPrivate p(doc);
  return p.print(&printer);
}

bool KatePrinter::printPreview(KateDocument *doc)
{
  QPrinter printer;
  KatePrinterPrivate p(doc);
  QPrintPreviewDialog preview(&printer);
  QObject::connect(&preview, SIGNAL(paintRequested(QPrinter *)), &p, SLOT(paint(QPrinter *)));
  return preview.exec();
}

//END KatePrinter

#include "kateprinter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
