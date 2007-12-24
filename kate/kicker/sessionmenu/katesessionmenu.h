/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2005-2006, Anders Lund <anders@alweb.dk>
 */

#ifndef _KateSessionMenu_h_
#define _KateSessionMenu_h_

#include <k3panelmenu.h>

class KateSessionMenu : public K3PanelMenu {
  Q_OBJECT
  public:
    explicit KateSessionMenu( QWidget *parent=0, const QStringList& args=QStringList() );
    ~KateSessionMenu();

  public slots:
    virtual void initialize();

  protected slots:
    virtual void slotExec( int id );

  private:
    QStringList m_sessions;
    QWidget *m_parent;
};

#endif // _KateSessionMenu_h_

// kate: space-indent on; indent-width 2; replace-tabs on;
