/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger (jowenn@kde.org)

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

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACEADAPTOR_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACEADAPTOR_H

#include <QtDBus/QtDBus>

namespace KTextEditor
{

class HighlightingInterface;

/// For documentation see HighlightingInterface
class HighlightingInterfaceAdaptor: public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface","org.kde.KTextEditor.Highlighting")
  Q_PROPERTY(unsigned int mode READ mode WRITE setMode)
  Q_PROPERTY(unsigned int modeCount READ modeCount)

  public:
    HighlightingInterfaceAdaptor (QObject *obj,HighlightingInterface *iface);
    virtual ~HighlightingInterfaceAdaptor ();
  public Q_SLOTS:
    unsigned int mode ();
    bool setMode (unsigned int mode);
    unsigned int modeCount ();
    QString modeName (unsigned int mode);
    QString sectionName (unsigned int mode);
  Q_SIGNALS:
    void modeChanged ();
  private: HighlightingInterface *m_iface;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
