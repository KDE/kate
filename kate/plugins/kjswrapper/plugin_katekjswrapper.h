/* This file is part of the KDE project
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _PLUGIN_KATE_KJS_WRAPPER_H_
#define _PLUGIN_KATE_KJS_WRAPPER_H_

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/document.h>
#include <kate/pluginconfiginterface.h>
#include <kate/pluginconfiginterfaceextension.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kate/view.h>

#include <kcombobox.h>
#include <kdialogbase.h>
#include <klibloader.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <qcheckbox.h>
#include <qvaluelist.h>
#include <kjs/value.h>
#include <kjs/object.h>
#include <qvbox.h>
#include <qptrdict.h>
#include <kxmlguiclient.h>
#include <qvaluelist.h>
#include <qguardedptr.h>

namespace Kate {
  class PluginConfigPage;
  namespace JS {
	class Bindings;
	class Management;
		class Application;
	class MainWindow;
  }
}


namespace KJSEmbed {
	class KJSEmbedPart;
	class JSFactory;
}

class KateKJSWrapperConfigPage;
class PluginKateKJSWrapperView;

namespace KMDI {
	class ToolViewAccessor;
}

class PluginKateKJSWrapper : public Kate::Plugin, 
                             Kate::PluginViewInterface, 
                             Kate::PluginConfigInterfaceExtension
{
  Q_OBJECT

  public:
    PluginKateKJSWrapper( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
    virtual ~PluginKateKJSWrapper();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

      Kate::View *kv;


    QPtrDict<class PluginKateKJSWrapperView> m_views;
    uint configPages () const;
    Kate::PluginConfigPage *configPage (uint , QWidget *w, const char *name=0);
    QString configPageName(uint) const;
    QString configPageFullName(uint) const;
    QPixmap configPagePixmap (uint /*number = 0*/, 
                              int /*size = K3Icon::SizeSmall*/) const;
    PluginKateKJSWrapperView *getViewObject(Kate::MainWindow *win);

  public slots:
    //void slotInsertCommand();
    //void slotAbort();
    void applyConfig( KateKJSWrapperConfigPage* );
    void slotWindowDestroyed();

  private:
	friend class Kate::JS::Management;
	friend class KateKJSWrapperConfigPage;
	KJSEmbed::KJSEmbedPart *m_part;
	Kate::JS::Bindings *m_kateAppBindings;
	//QValueList<KJS::Value> m_configPageFactories;
	KJS::Value m_configPageFactories;
	KJS::Value m_toolViewConstructors;
	KJS::Value m_newWindowHandler;
	KJS::Value m_removeWindowHandler;
	QString m_scriptname;
  };


/** Config page for the plugin. */
class KateKJSWrapperConfigPage : public Kate::PluginConfigPage
{
  Q_OBJECT
  friend class PluginKateKJSWrapper;

  public:
    KateKJSWrapperConfigPage(KJS::Object pageConstructor,PluginKateKJSWrapper* parent = 0L, QWidget *parentWidget = 0L);
    ~KateKJSWrapperConfigPage() {};

    /** Reimplemented from Kate::PluginConfigPage
     * just emits configPageApplyRequest( this ).
     */
    void apply();

    void reset ();
    void defaults ();

  signals:
    /** Ask the plugin to set initial values */
    void configPageApplyRequest( KateKJSWrapperConfigPage* );
    /** Ask the plugin to apply changes */
    void configPageInitRequest( KateKJSWrapperConfigPage* );

  private:
     KJS::Object m_pageObject;
     PluginKateKJSWrapper *m_plugin;
  };

class PluginKateKJSWrapperView : public KXMLGUIClient
{
  public:

    virtual ~PluginKateKJSWrapperView();

  private:
    friend class PluginKateKJSWrapper;
    friend class Kate::JS::Application;
    friend class Kate::JS::MainWindow;
    void removeFromWindow();

    Kate::MainWindow *win;
    KJS::Object winObj;
    KJS::Object actionCollectionObj;
    QValueList<QGuardedPtr<KMDI::ToolViewAccessor> > toolviews;
};



namespace Kate {
	namespace JS {
		class ToolView: public QVBox {
			Q_OBJECT
		public:
			ToolView(KJS::Object constr, KJS::ExecState *exec, KJSEmbed::JSFactory *factory, KJS::List parameters, const char * name);
			virtual ~ToolView();
		private:
			KJS::Object handler;
		};
	}

}

#endif // _PLUGIN_KATE_KJS_WRAPPER_H_

