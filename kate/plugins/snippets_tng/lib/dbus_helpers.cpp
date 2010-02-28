/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "dbus_helpers.h"
#include <QDBusConnectionInterface>
#include <QDomDocument>
#include <kdebug.h>

namespace KTextEditor {
  namespace CodesnippetsCore {
#ifdef SNIPPET_EDITOR
  namespace Editor {
    
    void  notifyTokenNewHandled(const QString& token, const QString& service, const QString& object, const QString& filePath) {
	    QString objpath_qstring=QString(service);
	    QByteArray objpath_bytestring=object.utf8();
	    QLatin1String objpath(objpath_bytestring.constData());
	    QDBusMessage m = QDBusMessage::createMethodCall (service,            
	    object, "org.kde.Kate.Plugin.SnippetsTNG.Repository", "tokenNewHandled");
	    QList<QVariant>args;
	    args<<QVariant(token);
	    args<<QVariant(filePath);
	    m.setArguments(args);
	    QDBusConnection::sessionBus().call (m);          
    }    
    
#endif

    void notifyRepos() {
    /*  QDBusConnectionInterface *interface=QDBusConnection::sessionBus().interface();
      if (!interface) return;
      QStringList serviceNames = interface->registeredServiceNames();
      foreach(const QString serviceName,serviceNames) {
	if (serviceName.startsWith("org.kde.kate-")) {
	  QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
	  QLatin1String("/KTECodesnippetsCore/Repository"), "org.kde.Kate.Plugin.SnippetsTNG.Repository", "updateSnippetRepository");
	  QDBusConnection::sessionBus().call (m);
	}
      }*/
      QDBusConnectionInterface *interface=QDBusConnection::sessionBus().interface();
      if (!interface) return;
      QStringList serviceNames = interface->registeredServiceNames();

      foreach(const QString serviceName,serviceNames)
      {
	//m_dbusServiceName=QString("org.kde.KTECodesnippetsCore-%1.%2").arg(getpid()).arg(++s_id);
	if (serviceName.startsWith("org.kde.ktecodesnippetscore-"))
	{
	    QString objpath_qstring=QString("/Repository");
	    QByteArray objpath_bytestring=objpath_qstring.utf8();
	    QLatin1String objpath(objpath_bytestring.constData());
	    QDBusMessage m = QDBusMessage::createMethodCall (serviceName,            
	    objpath, "org.kde.Kate.Plugin.SnippetsTNG.Repository", "updateSnippetRepository");
	    QDBusConnection::sessionBus().call (m);          
	}
      }
    }
#ifdef SNIPPET_EDITOR
  }
#endif
  }
}