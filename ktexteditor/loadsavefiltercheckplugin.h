/* This file is part of the KDE libraries
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
      [katedocument.cpp, LGPL v2 only]

================RELICENSED=================

    This file is part of the KDE libraries
    Copyright (C) 2008 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_LOADSAVEFILTERCHECKPLUGIN_H
#define KDELIBS_KTEXTEDITOR_LOADSAVEFILTERCHECKPLUGIN_H

//BEGIN includes
#include <ktexteditor/document.h>
#include <QtCore/QObject>
//END  includes

namespace KTextEditor
{

class LoadSaveFilterCheckPluginPrivate;

/**
* \brief Plugin for load/save filtering
*/ 
class KTEXTEDITOR_EXPORT LoadSaveFilterCheckPlugin:public QObject {
  Q_OBJECT
  public:
    LoadSaveFilterCheckPlugin(QObject *parent); //expect parent to be 0
    virtual ~LoadSaveFilterCheckPlugin();
    /*this one is called once everything is set up for saving (especially the encoding has been determind (example: used for checking python source encoding headers))
    return true if saving should continue, false to abort
    */
    virtual bool preSavePostDialogFilterCheck(KTextEditor::Document *document) =0;
    /*this one is called after the file has been saved
      return true if following plugins should be handled*/
    virtual bool postSaveFilterCheck(KTextEditor::Document *document, bool saveas) =0;
    /*this one is called once the document has been completely loaded and configured (encoding,highlighting, ...))*/
    virtual void postLoadFilter(KTextEditor::Document *document) =0;
  private:
    class LoadSaveFilterCheckPluginPrivate* const d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
