/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_CMDLINE_H__
#define __KATE_CMDLINE_H__

#include <klineedit.h>

class KateView;

class KateCmdLine : public KLineEdit
{
  Q_OBJECT

  public:
    KateCmdLine (KateView *view);
    virtual ~KateCmdLine ();

  private slots:
    void slotReturnPressed ( const QString& cmd );
    void hideMe ();

  protected:
    void focusInEvent ( QFocusEvent *ev );
    void keyPressEvent( QKeyEvent *ev );

  private:
    KateView *m_view;
    bool m_msgMode;
    QString m_oldText;
};

#endif
