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

#ifndef __ktexteditor_configinterfaceextension_h__
#define __ktexteditor_configinterfaceextension_h__

#include <qwidget.h>
#include <qpixmap.h>
#include <kicontheme.h>

namespace KTextEditor
{

class KTEXTEDITOR_EXPORT ConfigPage : public QWidget
{
  Q_OBJECT

  public:
    ConfigPage ( QWidget *parent=0, const char *name=0 );
    virtual ~ConfigPage ();

  //
  // slots !!!
  //
  public:
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

/**
* This is an interface to extend the configuration of the
* Document, Plugin and PluginViewInterface classes.
*/
class KTEXTEDITOR_EXPORT ConfigInterfaceExtension
{
  friend class PrivateConfigInterfaceExtension;

  public:
    ConfigInterfaceExtension();
    virtual ~ConfigInterfaceExtension();

    unsigned int configInterfaceExtensionNumber () const;
    
  protected:  
    void setConfigInterfaceExtensionDCOPSuffix (const QCString &suffix); 

  //
  // slots !!!
  //
  public:
    /**
      Number of available config pages
    */
    virtual uint configPages () const = 0;
    
    /**
      returns config page with the given number,
      config pages from 0 to configPages()-1 are available
      if configPages() > 0
    */ 
    virtual ConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name=0 ) = 0;
  
    virtual QString configPageName (uint number = 0) const = 0;
    virtual QString configPageFullName (uint number = 0) const = 0;
    virtual QPixmap configPagePixmap (uint number = 0, int size = KIcon::SizeSmall) const = 0;
    
    
  private:
    class PrivateConfigInterfaceExtension *d;
    static unsigned int globalConfigInterfaceExtensionNumber;
    unsigned int myConfigInterfaceExtensionNumber;
};

class Document;
class Plugin;
class ViewPlugin;

KTEXTEDITOR_EXPORT ConfigInterfaceExtension *configInterfaceExtension (Document *doc);
KTEXTEDITOR_EXPORT ConfigInterfaceExtension *configInterfaceExtension (Plugin *plugin);

}

#endif
