/* This file is part of the KDE libraries
  Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

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

#include "templateinterface.h"
#include "document.h"
#include <stdaddressbook.h>
#include <addressee.h>
#include <qstring.h>
#include <klocale.h>
#include <kglobal.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <kmessagebox.h>
#include <kcalendarsystem.h>
#include <unistd.h>

#include <kdebug.h>

using namespace KTextEditor;

unsigned int TemplateInterface::globalTemplateInterfaceNumber = 0;

TemplateInterface::TemplateInterface()
{
  myTemplateInterfaceNumber = globalTemplateInterfaceNumber++;
}

TemplateInterface::~TemplateInterface()
{}

uint TemplateInterface::templateInterfaceNumber () const
{
  return myTemplateInterfaceNumber;
}

void TemplateInterface::setTemplateInterfaceDCOPSuffix ( const QCString &suffix )
{}

#define INITKABC do { \
  if (addrBook==0) { \
    addrBook=KABC::StdAddressBook::self(); \
    userAddress=addrBook->whoAmI(); \
    if (userAddress.isEmpty()) { \
      /*instead of sorry add he posibility to launch kaddressbook here*/ \
      KMessageBox::sorry(parentWindow,i18n("The template needs information about you, please set your identity in your addressbook"));\
      return false; \
    } \
  } \
} while(false)

bool TemplateInterface::expandMacros( QMap<QString, QString> &map, QWidget *parentWindow )
{
  KABC::StdAddressBook *addrBook = 0;
  KABC::Addressee userAddress;
  QDateTime datetime = QDateTime::currentDateTime();
  QDate date = datetime.date();
  QTime time = datetime.time();

  QMap<QString,QString>::Iterator it;
  for ( it = map.begin(); it != map.end(); ++it )
  {
    QString placeholder = it.key();
    if ( map[ placeholder ].isEmpty() )
    {
      if ( placeholder == "index" ) map[ placeholder ] = "i";
      else if ( placeholder == "loginname" )
      {}
      else if ( placeholder == "firstname" )
      {
        INITKABC;
        map[ placeholder ] = userAddress.givenName();
      }
      else if ( placeholder == "lastname" )
      {
        INITKABC;
        map[ placeholder ] = userAddress.familyName();
      }
      else if ( placeholder == "fullname" )
      {
        INITKABC;
        map[ placeholder ] = userAddress.assembledName();
      }
      else if ( placeholder == "email" )
      {
        INITKABC;
        map[ placeholder ] = userAddress.preferredEmail();
      }
      else if ( placeholder == "date" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatDate( date, true );
      }
      else if ( placeholder == "time" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatTime( time, true, false );
      }
      else if ( placeholder == "year" )
      {
        map[ placeholder ] = KGlobal::locale() ->calendar() ->yearString( date, false );
      }
      else if ( placeholder == "month" )
      {
        map[ placeholder ] = QString::number( KGlobal::locale() ->calendar() ->month( date ) );
      }
      else if ( placeholder == "day" )
      {
        map[ placeholder ] = QString::number( KGlobal::locale() ->calendar() ->day( date ) );
      }
      else if ( placeholder == "hostname" )
      {
        char hostname[ 256 ];
        hostname[ 0 ] = 0;
        gethostname( hostname, 255 );
        hostname[ 255 ] = 0;
        map[ placeholder ] = QString::fromLocal8Bit( hostname );
      }
      else if ( placeholder == "cursor" )
      {
        map[ placeholder ] = "|";
      }
      else map[ placeholder ] = placeholder;
    }
  }
  return true;
}

bool TemplateInterface::insertTemplateText ( uint line, uint column, const QString &templateString, const QMap<QString, QString> &initialValues, QWidget *parentWindow )
{
  QMap<QString, QString> enhancedInitValues( initialValues );

  QRegExp rx( "[$%]\\{([^}\\s]+)\\}" );
  rx.setMinimal( true );
  int pos = 0;
  int opos = 0;

  while ( pos >= 0 )
  {
    pos = rx.search( templateString, pos );

    if ( pos > -1 )
    {
      if ( ( pos - opos ) > 0 )
      {
        if ( templateString[ pos - 1 ] == '\\' )
        {
          pos = opos = pos + 1;
          continue;
        }
      }
      QString placeholder = rx.cap( 1 );
      if ( ! enhancedInitValues.contains( placeholder ) )
        enhancedInitValues[ placeholder ] = "";

      pos += rx.matchedLength();
      opos = pos;
    }
  }

  return expandMacros( enhancedInitValues, parentWindow )
         && insertTemplateTextImplementation( line, column, templateString, enhancedInitValues, parentWindow );
}



TemplateInterface *KTextEditor::templateInterface ( KTextEditor::Document *doc )
{
  if ( !doc )
    return 0;

  return static_cast<TemplateInterface*>( doc->qt_cast( "KTextEditor::TemplateInterface" ) );
}

