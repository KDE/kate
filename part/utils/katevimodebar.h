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

#ifndef KATE_VI_MODE_BAR_H
#define KATE_VI_MODE_BAR_H

#include "kateviewhelpers.h"
#include "kateview.h"

class QLabel;
class QTimer;
class KateViNormalMode;
class KateViInsertMode;
class KateViVisualMode;

class KateViModeBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViModeBar(KateView* view, QWidget* parent = 0);
  ~KateViModeBar();

  void updateViMode(ViMode mode);
  void updatePartialCommand(const QString &cmd);
  void showMessage(const QString &msg);
  void showErrorMessage(const QString &msg);
  void clearMessage();

private Q_SLOTS:
  void _clearMessage();

private:
  // move to some common place? seems it may be useful for others.
  QString modeToString(ViMode mode) const;

private:
  QLabel* m_labelStatus;
  QLabel* m_labelMessage;
  QLabel* m_labelCommand;
  QTimer* m_timer;
};

#endif
