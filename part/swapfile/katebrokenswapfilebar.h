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

#ifndef _KATE_BROKEN_SWAP_H__
#define _KATE_BROKEN_SWAP_H__

#include "kateviewhelpers.h"

class KateView;

namespace Ui {
  class BrokenSwapFileWidget;
}

class KateBrokenSwapFileBar : public KateViewBarWidget
{
  Q_OBJECT

  public:
    explicit KateBrokenSwapFileBar(KateView *view, QWidget *parent = 0);
    ~KateBrokenSwapFileBar ();

  protected Q_SLOTS:
    void showWhatsThis(const QString&);

  private:
    KateView *const m_view;
    Ui::BrokenSwapFileWidget *m_ui;
};

#endif //_KATE_BROKEN_SWAP_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
