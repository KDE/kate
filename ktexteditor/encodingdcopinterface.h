#ifndef Encoding_DCOP_INTERFACE_H
#define Encoding_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>
//#include "editdcopinterface.moc"
namespace KTextEditor
{
	class EncodingInterface;
	/**
	This is the main interface to the @ref EncodingInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref EncodingInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class EncodingDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentEncodingInterface - The parent @ref EncodingInterface object
		that will provide us with the functions for the interface.
		*/
		EncodingDCOPInterface( EncodingInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		*/
		virtual ~EncodingDCOPInterface();
	k_dcop:
		uint encodingInterfaceNumber ();
		void setEncoding (QString e) ;
		QString encoding();
   
	private:
		EncodingInterface *m_parent;
	};
};
#endif


