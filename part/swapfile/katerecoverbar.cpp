/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *  Copyright (C) 2010 Diana-Victoria Tiriplica <diana.tiriplica@gmail.com>
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

#include "katerecoverbar.h"
#include "ui_recoverwidget.h"
#include "kateswapfile.h"
#include "kateview.h"

#include <QWhatsThis>

//BEGIN KateRecoverBar
KateRecoverBar::KateRecoverBar(KateView *view, QWidget *parent)
  : KateViewBarWidget( false, parent )
  , m_view( view )
{
  Ui::RecoverWidget* ui = new Ui::RecoverWidget();
  ui->setupUi( centralWidget() );
  
  // clicking on the "Help" link pops up the content as what's this
  connect(ui->lblSwap, SIGNAL(linkActivated(const QString&)),
          this, SLOT(showWhatsThis(const QString&)));

  // set icons, but keep text from ui file
  ui->btnRecover->setGuiItem(KGuiItem(ui->btnRecover->text(), KIcon("edit-redo")));
  ui->btnDiscard->setGuiItem(KStandardGuiItem::discard());
  ui->lblIcon->setPixmap(KIcon("dialog-warning").pixmap(48, 48));

  // use queued connections because this (all) KateRecoverBar widgets are deleted
  connect(ui->btnRecover, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(recover()), Qt::QueuedConnection);
  connect(ui->btnDiscard, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(discard()), Qt::QueuedConnection);
  connect(ui->btnDiff, SIGNAL(clicked()), this, SLOT(viewDiff()));
}

void KateRecoverBar::showWhatsThis(const QString& text)
{
  QWhatsThis::showText(QCursor::pos(), text);
}

void KateRecoverBar::viewDiff()
{
}

//END KateRecoverBar

// kate: space-indent on; indent-width 2; replace-tabs on;
