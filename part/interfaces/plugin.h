/***************************************************************************
                          plugin.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/* **************************************************************************
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef _KATE_PLUGIN_INCLUDE_
#define _KATE_PLUGIN_INCLUDE_

#include <qobject.h>
#include <kxmlguiclient.h>

#include <qptrlist.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qwidget.h>

namespace Kate
{

/** Plugins that wish to have a page in the kate config dialog must create a widget
    inheriting from this class and implement the applyConfig() method.
*/
class PluginConfigPage : public QWidget
{
  Q_OBJECT

  friend class Plugin;
  friend class PluginView;

  public:
    PluginConfigPage (QObject *parent = 0L, QWidget *wparent = 0L);
    virtual ~PluginConfigPage ();

    /** Reimplement to apply the config of the page to the plugin and save it */
    virtual void applyConfig () { ; };

    class Plugin *myPlugin;
};

/** Base class for plugin views.
    Use the constructor from the plugins @ref createView method.
*/
class PluginView : public QObject, virtual public KXMLGUIClient
{
  Q_OBJECT

  friend class Plugin;
  friend class PluginConfigPage;

  public:
    PluginView (class Plugin *plugin = 0L, class MainWindow *mainwin = 0L);
    virtual ~PluginView ();

    /** Set the xmlGUI rc file for the plugin. */
    void setXML (QString);

    class Plugin *myPlugin;
    class MainWindow *myMainWindow;
};


/** The base class for all Kate plugins.

*/
class Plugin : public QObject
{
  Q_OBJECT

  friend class PluginView;
  friend class PluginConfigPage;

  public:
    Plugin (QObject *parent= 0L, const char *name = 0L);
    virtual ~Plugin ();

    /** Called by the plugin manager to create GUI elements for the plugin.
        This method must create a Kate::PluginView and the required actions
        and sidebar widgets as well as setting the ui XML file.
<pre>
Kate::PluginView* myPlugin::createView()
{
  Kate::PluginView *view = new Kate::PluginView( this, win );
  // ... initialize GUI elements
  return view;
}
</pre>
    */
    virtual PluginView *createView (class MainWindow *) { return 0L; };

    /** Plugins that has no GUI elements must reimplement this to return false.
    */
    virtual bool hasView () { return true; };

    /** Plugins that has a config page must implement this method. It is called by
        the plugin manager when the config dialog is created.
        It must create and initialize the config page and return a pointer
    */
    virtual PluginConfigPage *createConfigPage (QWidget *) { return 0L; };

    /** Plugins that has a config page must remplement this method to return true.
    */
    virtual bool hasConfigPage () { return false; };

    /** Plugins that has a config page must reimplement this method to return
        a suitable name for the page
    */
    virtual class QString configPageName() { return 0L; };

    /** Plugins that has a config page must reimplement this method to return
        a suitable title for the page. The title is displayed on the top of the config page.
    */
    virtual class QString configPageTitle() { return 0L; };

    /** Plugins that has a config page may reimplement this method to return
        a suitable icon for the page. The icon is displayed in the config dialog tree.
    */
    virtual class QPixmap configPageIcon() { return 0L; };

    QPtrList<PluginView> viewList;

    /** A pointer to the application object.
    Allows the plugin to acces the document and view managers as well as the main window.
    */
    class Application *myApp;
};

};

#endif
