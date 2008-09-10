/* This file is part of the KDE libraries
 * Copyright (C) 2008 Dmitry Suzdalev <dimsuz@gmail.com>
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
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

#include "katevimodebar.h"
#include "kateviinputmodemanager.h"
#include "katevinormalmode.h"
#include "katevivisualmode.h"
#include "kateviinsertmode.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

#include "klocale.h"

KateViModeBar::KateViModeBar(KateView* view, QWidget* parent)
: KateViewBarWidget(false, view, parent),
  m_labelStatus(new QLabel(this)),
  m_labelCommand(new QLabel(this))
{
  QHBoxLayout *lay = static_cast<QHBoxLayout*>(layout());
  lay->addWidget(m_labelStatus);
  lay->addStretch(1);
  lay->addWidget(m_labelCommand);
  lay->addSpacing(30);

  // otherwise the command will look 'jumpy' as new symbols are added to it
  // 50 pix should be enough i think
  m_labelCommand->setFixedWidth(50);
}

KateViModeBar::~KateViModeBar()
{

}

void KateViModeBar::updateViMode(ViMode mode)
{
  m_labelStatus->setText(modeToString(mode));
}

void KateViModeBar::updatePartialCommand(const QString &cmd)
{
  m_labelCommand->setText(cmd);
}
QString KateViModeBar::modeToString(ViMode mode) const
{
  QString modeStr;
  switch (mode) {
    case InsertMode:
      modeStr = i18n("VI: INSERT MODE");
      break;
    case NormalMode:
      modeStr = i18n("VI: NORMAL MODE");
      break;
    case VisualMode:
      modeStr = i18n("VI: VISUAL");
      break;
    case VisualLineMode:
      modeStr = i18n("VI: VISUAL LINE");
      break;
  }
  return modeStr;
}
