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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __ktexteditor_configinterface_h__
#define __ktexteditor_configinterface_h__

#include <kdelibs_export.h>

class QCString;
class KConfig;

namespace KTextEditor
{

/**
* This is an interface for accessing the configuration of the
* Document and Plugin classes.
*/
class KTEXTEDITOR_EXPORT ConfigInterface
{
  friend class PrivateConfigInterface;

  public:
    ConfigInterface();
    virtual ~ConfigInterface();

    unsigned int configInterfaceNumber () const;
    
  protected:  
    void setConfigInterfaceDCOPSuffix (const QCString &suffix); 

  //
  // slots !!!
  //
  public:    
    /**
      Read/Write the config to the standard place where this editor
      part saves it config, say: read/save default values for that
      editor part, which means for all current open documents
    */
    virtual void readConfig () = 0;
    virtual void writeConfig () = 0;           
                                                                         
    /**
      Read/Write the config of the part to a given kconfig object
      to store the settings in a different place than the standard
    */
    virtual void readConfig (KConfig *) = 0;
    virtual void writeConfig (KConfig *) = 0;
    
    /**
      Read/Write session config of only this document/view/plugin 
    */
    virtual void readSessionConfig (KConfig *) = 0;
    virtual void writeSessionConfig (KConfig *) = 0;
    
    /**
      Shows a config dialog for the part, changes will be applied
      to the part, but not saved anywhere automagically, call
      writeConfig to save it
    */
    virtual void configDialog () = 0;

  private:
    class PrivateConfigInterface *d;
    static unsigned int globalConfigInterfaceNumber;
    unsigned int myConfigInterfaceNumber;
};

class Plugin;
class Document;

KTEXTEDITOR_EXPORT ConfigInterface *configInterface (Document *doc);
KTEXTEDITOR_EXPORT ConfigInterface *configInterface (Plugin *plugin);

}

#endif
