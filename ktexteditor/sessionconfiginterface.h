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

#ifndef __ktexteditor_sessionconfiginterface_h__
#define __ktexteditor_sessionconfiginterface_h__

class KConfig;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document/View/Plugin/ViewPlugin classes !!!
*/
class SessionConfigInterface
{
  friend class PrivateSessionConfigInterface;

  public:
    SessionConfigInterface();
    virtual ~SessionConfigInterface();

    unsigned int configInterfaceNumber () const;

  //
  // slots !!!
  //
  public:    
    /**
      Read/Write the config to the standard place where this editor
      part saves it config, say: read/save default values for that
      editor part
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
    class PrivateSessionConfigInterface *d;
    static unsigned int globalSessionConfigInterfaceNumber;
    unsigned int mySessionConfigInterfaceNumber;
};

SessionConfigInterface *sessionSessionConfigInterface (class Document *doc);
SessionConfigInterface *sessionSessionConfigInterface (class View *view);
SessionConfigInterface *sessionSessionConfigInterface (class Plugin *plugin);
SessionConfigInterface *sessionSessionConfigInterface (class ViewPlugin *plugin);

};

#endif
