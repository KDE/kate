/* This file is part of the KDE libraries
  Copyright (C) 2004, 2010 Joseph Wenninger <jowenn@kde.org>

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

#include "templateinterface.h"
#include "document.h"
#include "view.h"
#include <QtCore/QString>
#include <klocale.h>
#include <kglobal.h>
#include <QtCore/QDate>
#include <QtCore/QRegExp>
#include <kmessagebox.h>
#include <kcalendarsystem.h>
#include <unistd.h>
#include <klibloader.h>

#include <kdebug.h>

#define DUMMY_VALUE "!KTE:TEMPLATEHANDLER_DUMMY_VALUE!"

using namespace KTextEditor;

bool TemplateInterface::expandMacros( QMap<QString, QString> &map, QWidget *parentWindow)
{
  QDateTime datetime = QDateTime::currentDateTime();
  QDate date = datetime.date();
  QTime time = datetime.time();
  typedef QString (*kabcbridgecalltype)(const QString&,QWidget *,bool *ok);
  kabcbridgecalltype kabcbridgecall=0;

  QStringList kabcitems;
  kabcitems<<"firstname"<<"lastname"<<"fullname"<<"email";

  QMap<QString,QString>::Iterator it;
  for ( it = map.begin(); it != map.end(); ++it )
  {
    QString placeholder = it.key();
    if ( map[ placeholder ].isEmpty() )
    {
      if ( placeholder == "index" ) map[ placeholder ] = "i";
      else if ( placeholder == "loginname" )
      {}
      else if (kabcitems.contains(placeholder))
      {
        if (kabcbridgecall==0)
        {
          KLibrary lib(QLatin1String("ktexteditorkabcbridge"));
          kabcbridgecall=(kabcbridgecalltype)lib.resolveFunction("ktexteditorkabcbridge");
          if (kabcbridgecall == 0)
          {
            KMessageBox::sorry(parentWindow,i18n("The template needs information about you, which is stored in your address book.\nHowever, the required plugin could not be loaded.\n\nPlease install the KDEPIM/Kontact package for your system."));
            return false;
          }
        }
        bool ok;
        map[ placeholder ] = kabcbridgecall(placeholder,parentWindow,&ok);
        if (!ok)
        {
          return false;
        }
      }
      else if ( placeholder == "date" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatDate( date, KLocale::ShortDate );
      }
      else if ( placeholder == "time" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatTime( time, true, false );
      }
      else if ( placeholder == "year" )
      {
        map[ placeholder ] = KGlobal::locale() ->calendar() ->yearString( date, KCalendarSystem::LongFormat );
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
        map[ placeholder ] = '|';
      }
      else if (placeholder== "selection" ) {
        //DO NOTHING, THE IMPLEMENTATION WILL HANDLE THIS
      }
      else map[ placeholder ] = placeholder;
    }
  }
  return true;
}

bool TemplateInterface::KTE_INTERNAL_setupIntialValues(const QString& templateString,QMap<QString,QString> *initialValues)
{
  QMap<QString, QString> enhancedInitValues( *initialValues );
  
  QRegExp rx( "[$%]\\{([^}\\r\\n]+)\\}" );
  rx.setMinimal( true );
  int pos = 0;
  int offset;
  QString initValue;
  while ( pos >= 0 )
  {
    bool initValue_specified=false;
    pos = rx.indexIn( templateString, pos );

    if ( pos > -1 )
    {
      offset = 0;
      while ( pos - offset > 0 && templateString[ pos - offset - 1 ] == '\\' ) {
        ++offset;
      }
      if ( offset % 2 == 1 ) {
        // match is escaped
        ++pos;
        continue;
      }
      QString placeholder = rx.cap( 1 );
      
      int pos_colon=placeholder.indexOf(":");
      int pos_slash=placeholder.indexOf("/");
      int pos_backtick=placeholder.indexOf("`");
      bool check_slash=false;
      bool check_colon=false;
      bool check_backtick=false;
      if ((pos_colon==-1) && ( pos_slash==-1)) {
        //do nothing
      } else if ( (pos_colon==-1) && (pos_slash!=-1)) {
        check_slash=true;
      } else if ( (pos_colon!=-1) && (pos_slash==-1)) {
        check_colon=true;
      } else {
        if (pos_colon<pos_slash)
          check_colon=true;
        else
          check_slash=true;
      }
      
      if ( (!check_slash) && (!check_colon) && (pos_backtick>=0) )
        check_backtick=true;
      
      if (check_slash) {                
        //in most cases it should not matter, but better safe then sorry.
        const int end=placeholder.length();
        int slashcount=0;
        int backslashcount=0;
        for (int i=0;i<end;i++) {
          if (placeholder[i]=='/') {
            if ((backslashcount%2)==0) slashcount++;
            if (slashcount==3) break;
            backslashcount=0;
          } else if (placeholder[i]=='\\')
            backslashcount++;
          else
            backslashcount=0; //any character terminates a backslash sequence
        }
        if (slashcount!=3) {
          const int tmpStrLength=templateString.length();
          for (int i=pos+rx.matchedLength();(slashcount<3) && (i<tmpStrLength);i++,pos++) {
              if (templateString[i]=='/') {
                if ((backslashcount%2)==0) slashcount++;
                backslashcount=0;
              } else if (placeholder[i]=='\\')
                backslashcount++;
              else
                backslashcount=0; //any character terminates a backslash sequence              
          }
        }
        //this is needed
        placeholder=placeholder.left(placeholder.indexOf("/"));
      } else if (check_colon) {
        initValue=placeholder.mid(pos_colon+1);
        initValue_specified=true;
        int  backslashcount=0;
        for (int i=initValue.length()-1;(i>=0) && (initValue[i]=='\\'); i--) {
          backslashcount++;
        }
        initValue=initValue.left(initValue.length()-((backslashcount+1)/2));
        if ((backslashcount % 2) ==1) {
          initValue+="}";
          const int tmpStrLength=templateString.length();
          backslashcount=0;
          for (int i=pos+rx.matchedLength();(i<tmpStrLength);i++,pos++) {
              if (templateString[i]=='}') {
                initValue=initValue.left(initValue.length()-((backslashcount+1)/2));
                if ((backslashcount%2)==0) break;
                backslashcount=0;
              } else if (placeholder[i]=='\\')
                backslashcount++;
              else
                backslashcount=0; //any character terminates a backslash sequence              
            initValue+=placeholder[i];
          }
        }
        placeholder=placeholder.left(placeholder.indexOf(":"));
      } else if (check_backtick) {
        placeholder=placeholder.left(pos_backtick);
      }
      
      if (placeholder.contains("@")) placeholder=placeholder.left(placeholder.indexOf("@"));
      if ( (! enhancedInitValues.contains( placeholder )) || (enhancedInitValues[placeholder]==DUMMY_VALUE)  ) {
        if (initValue_specified) {
          enhancedInitValues[placeholder]=initValue;
        } else {
          enhancedInitValues[ placeholder ] = DUMMY_VALUE;
        }
      }
      pos += rx.matchedLength();
    }
  }

  kDebug()<<"-----------------------------------";
  for (QMap<QString,QString>::iterator it=enhancedInitValues.begin();it!=enhancedInitValues.end();++it) {
    kDebug()<<"key:"<<it.key()<<" init value:"<<it.value();
    if (it.value()==DUMMY_VALUE) it.value()="";
  }
  kDebug()<<"-----------------------------------";
  if (!expandMacros( enhancedInitValues, dynamic_cast<QWidget*>(this) ) ) return false;
  *initialValues=enhancedInitValues;
  return true;
}  

bool TemplateInterface::insertTemplateText ( const Cursor& insertPosition, const QString &templateString, const QMap<QString, QString> &initialValues) {
  QMap<QString,QString> enhancedInitValues(initialValues);
  if (!KTE_INTERNAL_setupIntialValues(templateString,&enhancedInitValues)) return false;
  return insertTemplateTextImplementation( insertPosition, templateString, enhancedInitValues);
}

// kate: space-indent on; indent-width 2; replace-tabs on;