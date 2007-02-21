/* This file is part of the KDE project
   Copyright (C) 2003 Ian Reinhart Geiser <geiseri@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _katedocmanager_adaptor_h_
#define _katedocmanager_adaptor_h_

#include <QtDBus/QtDBus>

class KateDocManager;

class KateDocManagerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kate.DocManager")

  public:
    KateDocManagerAdaptor (KateDocManager *dm);

  public Q_SLOTS:

    //QDBusObjectPath document (uint n);

    //QDBusObjectPath activeDocument ();

    bool isOpen (QString url);

    uint documents ();

    //QDBusObjectPath openUrl (QString url, QString encoding);

    bool closeDocument (uint n);

    bool closeAllDocuments ();

  private:
    KateDocManager *m_dm;
};
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

