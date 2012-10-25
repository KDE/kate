/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010-2012 Dominik Haumann <dhaumann@kde.org>
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
#include "kateview.h"

#include <kmessagewidget.h>
#include <KStandardGuiItem>
#include <klocale.h>

#include <QHBoxLayout>

//BEGIN KateBrokenSwapFileBar
KateBrokenSwapFileBar::KateBrokenSwapFileBar(KateView *view, QWidget *parent)
  : KateViewBarWidget( false, parent )
  , m_view ( view )
{
  KMessageWidget* messageWidget = new KMessageWidget(centralWidget());
  messageWidget->setMessageType(KMessageWidget::Warning);
  messageWidget->setCloseButtonVisible(false);
  messageWidget->setWordWrap(false);
  messageWidget->setText(i18n("Could not recover data. The swap file was probably incomplete."));

  QAction* closeAction = new QAction(KStandardGuiItem::close().icon(), i18n("Close"), messageWidget);
  messageWidget->addAction(closeAction);

  QHBoxLayout* boxLayout = new QHBoxLayout(centralWidget());
  boxLayout->addWidget(messageWidget);

  // use queued connections because this (all) KateRecoverBar widgets are deleted
  connect(closeAction, SIGNAL(triggered()), this, SIGNAL(hideMe()), Qt::QueuedConnection);

  messageWidget->show();
}

KateBrokenSwapFileBar::~KateBrokenSwapFileBar ()
{
}

//END KateBrokenSwapFileBar

// kate: space-indent on; indent-width 2; replace-tabs on;
