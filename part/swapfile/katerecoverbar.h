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

#ifndef _KATE_RECOVER_H__
#define _KATE_RECOVER_H__

#include "kateviewhelpers.h"

#include <QByteArray>

class KProcess;
class KateView;
class KateDocument;

namespace Ui {
  class RecoverWidget;
}

class KateRecoverBar : public KateViewBarWidget
{
  Q_OBJECT

  public:
    explicit KateRecoverBar(KateView *view, QWidget *parent = 0);
    ~KateRecoverBar ();

  protected Q_SLOTS:
    void showWhatsThis(const QString&);
    void viewDiff();

  private:
    KateView *const m_view;
    Ui::RecoverWidget *m_ui;

  protected Q_SLOTS:
    void slotDataAvailable();
    void slotDiffFinished();

  private:
    KProcess* m_proc;
    QByteArray m_diffContent;
};

#endif //_KATE_RECOVER_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
