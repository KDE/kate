/* This file is part of the KDE libraries
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>

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

   $Id$
*/


#ifndef _INSERT_FILE_PLUGIN_H_
#define _INSERT_FILE_PLUGIN_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>

#include <kxmlguiclient.h>
#include <qobject.h>
#include <jobclasses.h>

class InsertFilePlugin : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface
{             
  Q_OBJECT

  public:
    InsertFilePlugin( QObject *parent = 0, 
                            const char* name = 0, 
                            const QStringList &args = QStringList() );
    virtual ~InsertFilePlugin();       
    
    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);
    
    
  private:
    QPtrList<class InsertFilePluginView> m_views;
};

class InsertFilePluginView : public QObject, public KXMLGUIClient
{
  Q_OBJECT
  public:
    InsertFilePluginView( KTextEditor::View *view, const char *name=0 );
    ~InsertFilePluginView() {};
  public slots:
    /* display a file dialog, and insert the chosen file */
    void slotInsertFile();
  private slots:
    void slotFinished( KIO::Job *job );
    //slotAborted( KIO::Job *job );
  private:
    void insertFile();
    QString _file;
    QString _tmpfile;
    KIO::FileCopyJob *_job;
};

#endif // _INSERT_FILE_PLUGIN_H_
