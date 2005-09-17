/* This file is part of the KDE project
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_CONFIGPAGE_H
#define KDELIBS_KTEXTEDITOR_CONFIGPAGE_H

#include <kdelibs_export.h>

#include <QWidget>

namespace KTextEditor
{

/**
 * This class represents a config page of an editor
 */
class KTEXTEDITOR_EXPORT ConfigPage : public QWidget
{
  Q_OBJECT

  public:
    ConfigPage ( QWidget *parent ) : QWidget (parent) {}
    virtual ~ConfigPage () {}

  public slots:
    /**
      Applies the changes to all documents
    */
    virtual void apply () = 0;

    /**
      Reset the changes
    */
    virtual void reset () = 0;

    /**
      Sets default options
    */
    virtual void defaults () = 0;

  signals:
    /**
      Emitted when something changes
    */
    void changed();
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
