/* This file is part of the KDE libraries
  Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

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

#include <stdaddressbook.h>
#include <addressee.h>
#include <QString>
#include <QWidget>
#include <QEventLoop>
#include <kprocess.h>
#include <kmessagebox.h>
#include <klocale.h>

static void ktexteditorkabcbridge_run_kaddressbook() {
	QEventLoop eventLoop;
	KProcess proc;
	proc<<"kaddressbook";
	proc<<"--nofork";
	QObject::connect(&proc, SIGNAL(processExited(KProcess*)),
		&eventLoop, SLOT(quit()));
	proc.start();
	eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

extern "C" {
KDE_EXPORT QString ktexteditorkabcbridge(const QString& placeholder, QWidget *widget,bool *ok) {
	KABC::StdAddressBook *addrBook=KABC::StdAddressBook::self();
	KABC::Addressee userAddress=addrBook->whoAmI();
	if (userAddress.isEmpty()) {
 		if (KMessageBox::questionYesNo(widget,
                          i18n("The template needs information about you. It looks like you did not set that information in the addressbook. Do you want to open the addressbook to enter the information ?"),
                          i18n("Missing personal information")) == KMessageBox::Yes) {
			ktexteditorkabcbridge_run_kaddressbook();
			userAddress=addrBook->whoAmI();
		}
		if (userAddress.isEmpty()) {
			KMessageBox::sorry(widget,i18n("The template needs information about you, please set your identity in your addressbook"));
			if (ok) *ok=false;
			return QString();
		}
	}
	if (ok) *ok=true;
	if ( placeholder == "firstname" )
		return userAddress.givenName();
	else if ( placeholder == "lastname" )
		return userAddress.familyName();
	else if ( placeholder == "fullname" )
		return userAddress.assembledName();
	else if ( placeholder == "email" )
		return userAddress.preferredEmail();
	else return QString();
}
}
