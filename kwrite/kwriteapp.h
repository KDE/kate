/* This file is part of the KDE project
Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

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

#ifndef __KWRITE_APP_H__
#define __KWRITE_APP_H__

#include "kwritemain.h"

#include <KApplication>
#include <ktexteditor/editor.h>
#include <ktexteditor/containerinterface.h>

#include <QList>

namespace KTextEditor
{
    class Document;
}

class KCmdLineArgs;

/**
 * KWrite Application
 * This class represents the core kwrite application object
 */
class KWriteApp
  : public KApplication
  , public KTextEditor::MdiContainer
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::MdiContainer)

  //
  // constructors & accessor to app object
  //
  public:
    /**
     * application constructor
     * @param args parsed command line args
     */
    KWriteApp(KCmdLineArgs *args);

    /**
     * application destructor
     */
    ~KWriteApp();

    /**
     * static accessor to avoid casting ;)
     * @return app instance
     */
    static KWriteApp *self ();

    /**
     * further initialization
     */
    void init();

    /**
     * Editor instance
     */
    KTextEditor::Editor *editor () { return m_editor; }

  //
  // KTextEditor::MdiContainer
  //
  public:
    virtual void setActiveView( KTextEditor::View * view );
    virtual KTextEditor::View * activeView();
    virtual KTextEditor::Document * createDocument();
    virtual bool closeDocument( KTextEditor::Document * doc );
    virtual KTextEditor::View * createView( KTextEditor::Document * doc );
    virtual bool closeView( KTextEditor::View * view );

  private:
    /**
     * kate's command line args
     */
    KCmdLineArgs *m_args;

    /**
     * known main windows
     */
    QList<KWrite*> m_mainWindows;

    /**
     * editor instance
     */
    KTextEditor::Editor *m_editor;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

