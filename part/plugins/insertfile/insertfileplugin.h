/***************************************************************************
    insertfileplugin.h
    Insert any readable file at cursor position
    
    begin                : Thu Jun 13 13:14:52 CEST 2002
    $Id:$
    copyright            : (C) 2002 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
    InsertFilePluginView( KTextEditor::View *view );
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
