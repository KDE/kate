#ifndef Undo_DCOP_INTERFACE_H
#define Undo_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>
//#include "editdcopinterface.moc"
namespace KTextEditor
{
	class UndoInterface;
	/**
	This is the main interface to the @ref UndoInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref UndoInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class UndoDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentUndoInterface - The parent @ref UndoInterface object
		that will provide us with the functions for the interface.
		*/
		UndoDCOPInterface( UndoInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		*/
		virtual ~UndoDCOPInterface();
	k_dcop:
		uint undoInterfaceNumber ();
		void undo ();
		void redo () ;
		void clearUndo () ;
		void clearRedo () ;
		uint undoCount () ;
		uint redoCount ();
		uint undoSteps () ;
		void setUndoSteps ( uint steps );
		void undoChanged ();
	
	
	private:
		UndoInterface *m_parent;
	};
};
#endif


