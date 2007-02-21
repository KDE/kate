#ifndef _KATE_JS_BINDINGS_H_
#define _KATE_JS_BINDINGS_H_

#include <kjsembed/jsbindingplugin.h>
#include <kjsembed/jsproxy_imp.h>
#include <kjsembed/jsobjectproxy.h>
#include <kjsembed/jsfactory.h>
#include <qptrdict.h>

class PluginKateKJSWrapper;

namespace Kate {
	namespace JS {


		struct ObjectEntry {
			KJS::Object obj;
		};

		class RefCountedObjectDict: public QObject, public QPtrDict<ObjectEntry> {
			Q_OBJECT
		public:
			RefCountedObjectDict(int size);
			void incRef();
			void decRef();
			KJS::Object jsObject(KJS::ExecState *exec, QObject *obj, KJSEmbed::JSObjectProxy *proxy);
		public slots:
			void removeSender();
		private:
			int m_usageCount;
		};


		class Bindings: public KJSEmbed::Bindings::JSBindingPlugin {
		public:
			Bindings(QObject *parent);
			virtual ~Bindings();
			KJS::Object createBinding(KJSEmbed::KJSEmbedPart *jspart, KJS::ExecState *exec, const KJS::List &args) const;
			void addBindings(KJS::ExecState *exec, KJS::Object &target) const;
		};

		class DocumentManager: public KJSEmbed::JSProxyImp {
		public:
			enum MethodID {
				Document,
				ActiveDocument,
				DocumentWithID,
				FindDocument,
				IsOpen,
				OpenURL,
				Documents,
				CloseDocument,
				CloseAllDocuments
			};
			virtual bool implementsCall() const { return true; }
		    	virtual KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
			static void addBindings(KJS::ExecState *exec, KJSEmbed::JSObjectProxy *proxy,KJS::Object &target);
			private:
			DocumentManager( KJS::ExecState *exec, int id, KJSEmbed::JSObjectProxy *parent, RefCountedObjectDict *dict );
			virtual ~DocumentManager();
		private:
			RefCountedObjectDict *m_dict;
			int m_id;
			KJSEmbed::JSObjectProxy *m_proxy;

		};

		class Management: public KJSEmbed::JSProxyImp {
		public:
			enum MethodID {
				AddConfigPage,
				SetConfigPages,
				RemoveConfigPage,
				SetWindowConfiguration,
				KJSConsole
			};
			Management( KJS::ExecState *exec, int id, class PluginKateKJSWrapper *kateplug);
			virtual bool implementsCall() const { return true; }
		    	virtual KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );

		private:
			int m_id;
			class PluginKateKJSWrapper *m_wrapper;

		};
		
		class Application: public KJSEmbed::JSProxyImp {
		public:
			enum MethodID {
				ToolView,
				WindowCount,
				Window,
				ActiveWindow,
				
			};
			virtual bool implementsCall() const { return true; }
		    	virtual KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
			static void addBindings(KJS::ExecState *exec, KJSEmbed::JSObjectProxy *proxy,KJS::Object &target);
			private:
			Application( KJS::ExecState *exec, int id, KJSEmbed::JSObjectProxy *parent, PluginKateKJSWrapper *plugin );
			~Application();
		private:
			int m_id;
			KJSEmbed::JSObjectProxy *m_proxy;
			PluginKateKJSWrapper *m_plugin;
		};

		class General: public KJSEmbed::JSProxyImp {
		public:
			enum MethodID {
				MethodMethods,
				MethodFields
			};
			virtual bool implementsCall() const { return true; }
		    	virtual KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
		    	virtual KJS::Value fieldz( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
		    	virtual KJS::Value methodz( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
			static void addBindings(KJS::ExecState *exec,KJSEmbed::JSObjectProxy *proxy,KJS::Object &target);
			private:
			General( KJS::ExecState *exec,KJS::Interpreter *interpreter,int id);

		private:
			int m_id;
			KJS::Interpreter *m_interpreter;
		};


		class MainWindow: public KJSEmbed::JSProxyImp {
		public:
			enum MethodID {
				ActionCollection
				
			};
			virtual bool implementsCall() const { return true; }
		    	virtual KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );
			static void addBindings(KJS::ExecState *exec, KJSEmbed::JSObjectProxy *proxy,KJS::Object &target);
			private:
			MainWindow( KJS::ExecState *exec, int id, KJSEmbed::JSObjectProxy *parent, PluginKateKJSWrapper *plugin );
			~MainWindow();
		private:
			int m_id;
			KJSEmbed::JSObjectProxy *m_proxy;
			PluginKateKJSWrapper *m_plugin;
		};


	}
}

#endif
