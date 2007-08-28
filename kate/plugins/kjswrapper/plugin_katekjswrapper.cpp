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
//BEGIN includes
#include "plugin_katekjswrapper.h"
#include "plugin_katekjswrapper.moc"
#include "bindings.h"

#include <kjsembed/kjsembedpart.h>
#include <kjsembed/jssecuritypolicy.h>
#include <kjsembed/jsfactory.h>
#include <kjsembed/jsconsolewidget.h>
#include <kjs/interpreter.h>
#include <kjs/value.h>
#include <kjs/object.h>
#include <kgenericfactory.h>
#include <kdebug.h>
#include <qlayout.h>
#include <kstandarddirs.h>
#include <kate/mainwindow.h>
#include <kate/toolviewmanager.h>
#include <k3dockwidget.h>
#include <qvbox.h>
//END includes

K_EXPORT_COMPONENT_FACTORY( katekjswrapperplugin, KGenericFactory<PluginKateKJSWrapper>( "katekjswrapper" ) )

PluginKateKJSWrapperView::~PluginKateKJSWrapperView() {
}

void PluginKateKJSWrapperView::removeFromWindow() {
      kDebug()<<"PluginKateKJSWrapperView::removeFromWindow";
      for (QValueList<QGuardedPtr<KMDI::ToolViewAccessor> >::iterator it=toolviews.begin();it!=toolviews.end();it=toolviews.begin()) {
		kDebug()<<"removeFromWindow: removing a toolview";
		KMDI::ToolViewAccessor* tva=(*it);
		toolviews.remove(it);
		win->toolViewManager()->removeToolView (tva);
      }
      win->guiFactory()->removeClient (this);
    }

PluginKateKJSWrapper::PluginKateKJSWrapper( QObject* parent, const char* name, const QStringList& list)
    : Kate::Plugin ( (Kate::Application *)parent, name ) {
    m_views.setAutoDelete(true);
    m_scriptname=list[0];
    m_kateAppBindings=new Kate::JS::Bindings(this);
    KJSEmbed::JSSecurityPolicy::setDefaultPolicy( KJSEmbed::JSSecurityPolicy::CapabilityAll );
    m_part = new KJSEmbed::KJSEmbedPart(this);
    KJS::Interpreter *js = m_part->interpreter();

    KJSEmbed::JSFactory *factory=m_part->factory();

/* factories for kate app classes */
    factory->addQObjectPlugin("Kate::Application",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::DocumentManager",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::MainWindow",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::PluginManager",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::InitPluginManager",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::ProjectManager",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::Project",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::ViewManager",m_kateAppBindings);
    factory->addQObjectPlugin("Kate::View",m_kateAppBindings);
/* toplevel objects*/
    KJS::Object appobj=m_part->addObject(Kate::application(),"KATE");
    js->globalObject().put( js->globalExec(),  "addConfigPage", KJS::Object(new Kate::JS::Management(js->globalExec(),Kate::JS::Management::AddConfigPage,this )));   
    js->globalObject().put( js->globalExec(),  "setConfigPages", KJS::Object(new Kate::JS::Management(js->globalExec(),Kate::JS::Management::SetConfigPages,this )));   
    js->globalObject().put( js->globalExec(),  "removeConfigPage", KJS::Object(new Kate::JS::Management(js->globalExec(),Kate::JS::Management::RemoveConfigPage,this )));   
    js->globalObject().put( js->globalExec(),  "setWindowConfiguration", KJS::Object(new Kate::JS::Management(js->globalExec(),Kate::JS::Management::SetWindowConfiguration,this )));   
    js->globalObject().put( js->globalExec(),  "KJSConsole", KJS::Object(new Kate::JS::Management(js->globalExec(),Kate::JS::Management::KJSConsole,this )));   

/*    KJSEmbed::JSConsoleWidget *w=m_part->view();
    w->show();
    //w->show();*/
    kDebug()<<"m_scriptname="<<m_scriptname;
    m_part->runFile(locate("appdata",QString("plugins/%1/%2.js").arg(m_scriptname).arg(m_scriptname)));
//"/home/jowenn/development/kde/cvs/kdeaddons/kate/kjswrapper/samples/test1.js");
}

PluginKateKJSWrapper::~PluginKateKJSWrapper()
{
	delete m_part;
	m_part=0;
}


uint PluginKateKJSWrapper::configPages () const {
        KJS::Interpreter *js = m_part->interpreter();
	KJS::ExecState *exec=js->globalExec();

    if (! (m_configPageFactories.isNull() || (m_configPageFactories.type()==KJS::NullType))) {
    	KJS::Object constrs=m_configPageFactories.toObject(exec);
	if (!exec->hadException()) {
		if (QString(constrs.classInfo()->className)=="Array") {
			kDebug()<<"config page  constructor array detected";
			uint size=constrs.get(exec,KJS::Identifier("length")).toInteger(exec);
			if (exec->hadException()) {
				exec->clearException(); 
				kDebug()<<"Error while retrieving array length";
			}
			else  return size;
		} else return 1;
	}
    }
	exec->clearException();
	return 0;
}


static KJS::Object getObj(KJS::Interpreter *js, KJS::Value mightBeArray, int id) {
	KJS::ExecState *exec=js->globalExec();
    	KJS::Object constrs=mightBeArray.toObject(exec);
	KJS::Value constr;
	if (!exec->hadException()) {
		if (QString(constrs.classInfo()->className)=="Array") {
			kDebug()<<"config page  constructor array detected";
			constr=constrs.get(exec,id);
		} else constr=mightBeArray;
	
	}
	return constr.toObject(js->globalExec());
}

QString PluginKateKJSWrapper::configPageName(uint id) const {
	if (id>=configPages()) return "";
        KJS::Interpreter *js = m_part->interpreter();

	KJS::Object constr=getObj(js,m_configPageFactories,id);

	KJS::Value o=constr.get(js->globalExec(),KJS::Identifier("name"));
	QString retVal( o.toString(js->globalExec()).qstring() );

	kDebug()<<"==============================================================================================";
	kDebug()<<"PluginKateKJSWrapper::configPageName: "<<retVal;
	kDebug()<<"==============================================================================================";
	js->globalExec()->clearException();
	return retVal;
}

QString PluginKateKJSWrapper::configPageFullName(uint id) const {
	if (id>=configPages()) return "";
        KJS::Interpreter *js = m_part->interpreter();

	KJS::Object constr=getObj(js,m_configPageFactories,id);

	KJS::Value o=constr.get(js->globalExec(),KJS::Identifier("fullName"));
	QString retVal( o.toString(js->globalExec()).qstring() );

	kDebug()<<"==============================================================================================";
	kDebug()<<"PluginKateKJSWrapper::configPageFullName: "<<retVal;
	kDebug()<<"==============================================================================================";
	js->globalExec()->clearException();
	return retVal;
}

QPixmap PluginKateKJSWrapper::configPagePixmap (uint /*number = 0*/,
                              int /*size = K3Icon::SizeSmall*/) const {
	return 0;
}


Kate::PluginConfigPage* PluginKateKJSWrapper::configPage (uint id, 
                                  QWidget *w, const char */*name*/) {
	kDebug()<<"PluginKateKJSWrapper::configPage";

	if (id>=configPages()) return 0;
        KJS::Interpreter *js = m_part->interpreter();

	KJS::Object constr=getObj(js,m_configPageFactories,id);

	if (js->globalExec()->hadException()) {
		kDebug()<<"PluginKateKJSWrapper::configPage: exit 1";
		js->globalExec()->clearException();
		return 0;
  	}
	
	if (!constr.implementsConstruct()) {
		kWarning()<<"config page factory has to be an object constructor";
		return 0;
	}

	KateKJSWrapperConfigPage *p=new KateKJSWrapperConfigPage(constr,this,w);	
	return (Kate::PluginConfigPage*)p;
/*
  KateKJSWrapperConfigPage* p = new KateKJSWrapperConfigPage(this, w);
  //init
  connect( p, SIGNAL(configPageApplyRequest(KateKJSWrapperConfigPage*)), 
           this, SLOT(applyConfig(KateKJSWrapperConfigPage*)) );
  return (Kate::PluginConfigPage*);*/
}




static KMDI::ToolViewAccessor *createToolView(KJSEmbed::JSFactory *factory,KJS::Interpreter *js, Kate::MainWindow *winN,KJS::Object win,KJS::Object viewConstructor) {
	KJS::List params;
        KJS::ExecState *exec = js->globalExec();
	params.append(win);				
	exec->clearException();
	int dockPos;
	if (!viewConstructor.implementsConstruct()) return 0;
	KJS::Value dockPosV=viewConstructor.get(exec,KJS::Identifier("startPosition"));
	if (exec->hadException()) {
		dockPos=KDockWidget::DockLeft;
		exec->clearException();
	} else {
		dockPos=dockPosV.toInteger(exec);
		if (exec->hadException()) {
			dockPos=KDockWidget::DockLeft;
			exec->clearException();
		}
	}
	QString viewName;
	KJS::Value viewNameV=viewConstructor.get(exec,KJS::Identifier("name"));
	if (exec->hadException()) {
		viewName="kjs_unknown";
		exec->clearException();
	} else {
		viewName=QString( viewNameV.toString(exec).qstring() );
		if (exec->hadException()) {
			viewName="kjs_unknown";
			exec->clearException();
		}
	}

	Kate::JS::ToolView *tv=new Kate::JS::ToolView(viewConstructor,exec,factory,params,viewName.utf8());
	//params.append(factory->createProxy(exec,tv));
	//KJS::Object otv=viewConstructor.construct(exec,params);
	if (exec->hadException()) {
		kDebug()<<"Error while calling constructor";
		delete tv;
		kDebug()<<exec->exception().toString(exec).qstring();
		exec->clearException();
		return 0;
	}
	KMDI::ToolViewAccessor *tva=winN->toolViewManager()->addToolView((KDockWidget::DockPosition)dockPos,tv,
		tv->icon()?(*(tv->icon())):QPixmap(),tv->caption());
    	kDebug()<<"****************************************************************************************";
	kDebug()<<"PluginKateKJSWrapper: Toolview has been added";
	kDebug()<<"****************************************************************************************";
	return tva;

}

PluginKateKJSWrapperView *PluginKateKJSWrapper::getViewObject(Kate::MainWindow *win) {
    PluginKateKJSWrapperView * view=m_views[win];
    if (!view) {
    	view=new PluginKateKJSWrapperView();
    	view->win=win;
	connect(win,SIGNAL(destroyed()),this,SLOT(slotWindowDestroyed()));
    	m_views.insert(win,view);
        KJS::Interpreter *js = m_part->interpreter();
        KJS::ExecState *exec = js->globalExec();
	view->actionCollectionObj=m_part->factory()->createProxy(exec,view->actionCollection());
        view->winObj=m_part->factory()->createProxy(exec,win);
    } else kDebug()<<"returning cached View/Window Object";
    return view;
}

void PluginKateKJSWrapper::addView(Kate::MainWindow *win)
{
    PluginKateKJSWrapperView * view=getViewObject(win); // this is needed to ensure correct caching the javascript object
    KJS::Interpreter *js = m_part->interpreter();
    KJS::ExecState *exec = js->globalExec();
    exec->clearException();
    kDebug()<<"****************************************************************************************";
    kDebug()<<"PluginKateKJSWrapper::addView";
    kDebug()<<"****************************************************************************************";
    kDebug()<<"checking for newWindowHandler";
    if (!m_newWindowHandler.isNull()) {
    	KJS::List param;
	param.append(view->winObj);
	KJS::Object newWinFunc=m_newWindowHandler.toObject(exec);
	if (exec->hadException()) {
		exec->clearException();
	} else {
		if (newWinFunc.implementsCall()) {
			newWinFunc.call(exec,js->globalObject(),param);
			if (exec->hadException()) {
				kDebug()<<"Error while calling newWindowHandler";
				exec->clearException();
			}
		}
	}
    }
    if (exec->hadException()) kDebug()<<"void PluginKateKJSWrapper::addView(Kate::MainWindow *win): exec had an exception - 1";

    kDebug()<<"checking for toolview constructors";
    if (! (m_toolViewConstructors.isNull() || (m_toolViewConstructors.type()==KJS::NullType))) {
    	KJS::Object constrs=m_toolViewConstructors.toObject(exec);
	if (!exec->hadException()) {
		if (QString(constrs.classInfo()->className)=="Array") {
			kDebug()<<"Toolview constructor array detected";
			int size=constrs.get(exec,KJS::Identifier("length")).toInteger(exec);
			if (exec->hadException()) {
				exec->clearException(); 
				kDebug()<<"Error while retrieving array length";
			}
			else {
				for (int i=0;i<size;i++) {
					KJS::Object constrO=constrs.get(exec,i).toObject(exec);
					if (exec->hadException()) {
						exec->clearException();
					} else {
						KMDI::ToolViewAccessor *w=createToolView(m_part->factory(),js,win,view->winObj,constrO);
						if (w) {
							view->toolviews.append(QGuardedPtr<KMDI::ToolViewAccessor>(w));
						}
						exec->clearException();
					}
				}
			}
		} else {
			kDebug()<<"Single toolview constructor detected";
			if (!constrs.implementsConstruct()) {
				kWarning()<<"wrong object type";
			} else {
				KMDI::ToolViewAccessor *w=createToolView(m_part->factory(),js,win,view->winObj,constrs);
				if (w) {
					view->toolviews.append(QGuardedPtr<KMDI::ToolViewAccessor>(w));
				}
				exec->clearException();
			}
		}
	
	}
    } else kDebug()<<"void PluginKateKJSWrapper::addView(Kate::MainWindow *win): no toolview constructors";


    if (exec->hadException()) kDebug()<<"void PluginKateKJSWrapper::addView(Kate::MainWindow *win): exec had an exception - 2";

    view->setComponentData (KComponentData("kate"));
    view->setXMLFile(QString("plugins/%1/%2.rc").arg(m_scriptname).arg(m_scriptname));
    win->guiFactory()->addClient (view);
}


void PluginKateKJSWrapper::slotWindowDestroyed() {
	m_views.remove((void*)sender());
}

void PluginKateKJSWrapper::removeView(Kate::MainWindow *win)
{
//here toolviews must not be destroyed. Only  cleanup functions called the view should be removed in the slot connected to the windows destroy signal only
    m_views[win]->removeFromWindow();
}



void PluginKateKJSWrapper::applyConfig( KateKJSWrapperConfigPage *p )
{
#if 0
  config->writeEntry( "Command History Length", p->sb_cmdhistlen->value() );
  // truncate the cmd hist if necessary?
  config->writeEntry( "Start In", p->rg_startin->id(p->rg_startin->selected()) );
  config->sync();
#endif
}

KateKJSWrapperConfigPage::KateKJSWrapperConfigPage(KJS::Object pageConstructor,PluginKateKJSWrapper* parent, 
                                                 QWidget *parentWidget)
  : Kate::PluginConfigPage( parentWidget ),m_plugin(parent)
{
	QVBoxLayout *l=new QVBoxLayout(this);
	l->setAutoAdd(true);
	l->activate();
	KJS::Interpreter *js = parent->m_part->interpreter();
	KJS::ExecState *exec = js->globalExec();
	exec->clearException();
	KJS::List param;
	param.append(parent->m_part->factory()->createProxy(exec,this,0));
	m_pageObject=pageConstructor.construct(exec,param);
}


static void callJS(KJSEmbed::KJSEmbedPart *p,KJS::Object o,const QString& funcName){
	KJS::Interpreter *js = p->interpreter();
	KJS::ExecState *exec = js->globalExec();
	KJS::List param;
	exec->clearException();
	KJS::Value funcV=o.get(exec,KJS::Identifier(funcName));
	if (exec->hadException()) {
#ifdef __GNUC__
#warning clear exception ?
#endif		
		return;
	}
	KJS::Object func=funcV.toObject(exec);
	if (exec->hadException()) {
#ifdef __GNUC__		
#warning clear exception ?
#endif		
		return;
	}
	if (func.implementsCall()) {
		func.call(exec,o,param);
		if (js->globalExec()->hadException()) {
#ifdef __GNUC__			
#warning clear exception ?
#endif			
			return;
		}
	}
}

void KateKJSWrapperConfigPage::apply()
{
	callJS(m_plugin->m_part,m_pageObject,"apply");
}

void KateKJSWrapperConfigPage::reset()
{
	callJS(m_plugin->m_part,m_pageObject,"reset");
}

void KateKJSWrapperConfigPage::defaults()
{
	callJS(m_plugin->m_part,m_pageObject,"defaults");
}


Kate::JS::ToolView::ToolView(KJS::Object constr, KJS::ExecState *exec, KJSEmbed::JSFactory *factory, KJS::List parameters, const char *name):QVBox(0,name) {
	parameters.append(factory->createProxy(exec,this));
	handler=constr.construct(exec,parameters);

}

Kate::JS::ToolView::~ToolView() {
}

