#ifndef ViewStatusMsg_DCOP_INTERFACE_H
#define ViewStatusMsg_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>
//#include "editdcopinterface.moc"
namespace KTextEditor
{
	class ViewStatusMsgInterface;
	/**
	This is the main interface to the @ref ViewStatusMsgInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref ViewStatusMsgInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class ViewStatusMsgDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentViewStatusMsgInterface - The parent @ref ViewStatusMsgInterface object
		that will provide us with the functions for the interface.
		*/
		ViewStatusMsgDCOPInterface( ViewStatusMsgInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		*/
		virtual ~ViewStatusMsgDCOPInterface();
	k_dcop:
		uint viewStatusMsgInterfaceNumber ();
		void viewStatusMsg (class QString msg) ;
	
	private:
		ViewStatusMsgInterface *m_parent;
	};
};
#endif


