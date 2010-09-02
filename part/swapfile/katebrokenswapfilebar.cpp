/*  This file is part of the Kate project.
 *
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

#include "katebrokenswapfilebar.h"
#include "ui_brokenswapfilewidget.h"
#include "kateview.h"

#include <QWhatsThis>

//BEGIN KateBrokenSwapFileBar
KateBrokenSwapFileBar::KateBrokenSwapFileBar(KateView *view, QWidget *parent)
  : KateViewBarWidget( false, parent )
  , m_view ( view )
  , m_ui (new Ui::BrokenSwapFileWidget())
{
  m_ui->setupUi( centralWidget() );

  // set warning icon
  m_ui->lblIcon->setPixmap(KIcon("dialog-warning").pixmap(64, 64));

  m_ui->btnOk->setGuiItem(KGuiItem(m_ui->btnOk->text(), KIcon("dialog-ok")));

  // clicking on the "Help" link pops up the content as what's this
  connect(m_ui->lblSwap, SIGNAL(linkActivated(const QString&)),
          this, SLOT(showWhatsThis(const QString&)));
  connect(m_ui->btnOk, SIGNAL(clicked()), this, SIGNAL(hideMe()));
}

KateBrokenSwapFileBar::~KateBrokenSwapFileBar ()
{
  delete m_ui;
}

void KateBrokenSwapFileBar::showWhatsThis(const QString& text)
{
  QWhatsThis::showText(QCursor::pos(), text);
}

//END KateBrokenSwapFileBar

// kate: space-indent on; indent-width 2; replace-tabs on;
