/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __ktexteditor_configinterface_h__
#define __ktexteditor_configinterface_h__

class QObject;
class KConfig;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class ConfigInterface
{
  friend class PrivateConfigInterface;

  public:
    ConfigInterface();
    virtual ~ConfigInterface();

    unsigned int configInterfaceNumber () const;

  //
  // slots !!!
  //
  public:
    /**
      Read/Write config of ALL current loaded documents 
    */
    virtual void readConfig () = 0;
    virtual void writeConfig () = 0;
    virtual void readConfig (KConfig *) = 0;
    virtual void writeConfig (KConfig *) = 0;
    
    /**
      Read/Write session config of only this document 
    */
    virtual void readSessionConfig (KConfig *) = 0;
    virtual void writeSessionConfig (KConfig *) = 0;
    
    /**
      Configures ALL current loaded documents 
    */
    virtual void configDialog () = 0;

  private:
    class PrivateConfigInterface *d;
    static unsigned int globalConfigInterfaceNumber;
    unsigned int myConfigInterfaceNumber;
};

ConfigInterface *configInterface (QObject *obj);

};

#endif
