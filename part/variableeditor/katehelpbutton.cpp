/* This file is part of the KDE project

   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katehelpbutton.h"

#include <kiconloader.h>
#include <ktoolinvocation.h>
#include <klocale.h>

KateHelpButton::KateHelpButton(QWidget* parent)
  : QToolButton(parent)
{
  setAutoRaise(true);
  setIconState(IconColored);
  setToolTip(i18n("Kate Handbook."));
  
  connect(this, SIGNAL(clicked()), SLOT(invokeHelp()));
}

KateHelpButton::~KateHelpButton()
{
}

void KateHelpButton::setIconState(IconState state)
{
  if (state == IconGrayscaled) {
    setIcon(SmallIcon("help-contents", 0, KIconLoader::DisabledState));
  } else if (state == IconHidden) {
    setIcon(QIcon());
  } else {
    setIcon(SmallIcon("help-contents", 0, KIconLoader::DefaultState));
  }

  update();
}

void KateHelpButton::invokeHelp()
{
  KToolInvocation::invokeHelp(m_section, "kate");
}

void KateHelpButton::setSection(const QString& section)
{
  m_section = section;
}


// kate: indent-width 2; replace-tabs on;
