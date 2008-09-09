/* This file is part of the KDE libraries
 * Copyright (C) 2008 Dmitry Suzdalev <dimsuz@gmail.com>
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
#ifndef _KATE_VI_MODE_BAR_H_
#define _KATE_VI_MODE_BAR_H_

#include "kateviewhelpers.h"
#include "kateview.h"

class QLabel;
class KateViNormalMode;
class KateViInsertMode;
class KateViVisualMode;

class KateViModeBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViModeBar(KateView* view, QWidget* parent = 0);
  ~KateViModeBar();

  // NOTE: if kate vi modes will ever have a base class this should become
  // one function which uses the base class and some of its virtual functions instead
  void updateAccordingToMode(const KateViNormalMode& mode);
  void updateAccordingToMode(const KateViInsertMode& mode);
  void updateAccordingToMode(const KateViVisualMode& mode);

private:
  // move to some common place? seems it may be useful for others.
  QString modeToString(/*ViMode*/int mode) const;

private:
  QLabel* m_labelStatus;
  QLabel* m_labelCommand;
};

#endif
