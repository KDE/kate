#ifndef DocumentInfo_DCOP_INTERFACE_H
#define DocumentInfo_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>

namespace KTextEditor
{
	class DocumentInfoInterface;
	/**
	This is the main interface to the @ref DocumentInfoInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref DocumentInfoInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/

	class DocumentInfoDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param Parent the parent @ref DocumentInfoInterface object
		that will provide us with the functions for the interface.
		@param name the QObject's name
		*/
		DocumentInfoDCOPInterface( DocumentInfoInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		*/
		virtual ~DocumentInfoDCOPInterface();
	k_dcop:
		QString mimeType();
		long  fileSize();
		QString niceFileSize();
		uint documentInfoInterfaceNumber ();
	private:
		DocumentInfoInterface *m_parent;
	};
}
#endif


