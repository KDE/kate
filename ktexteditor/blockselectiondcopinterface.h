#ifndef BlockSelection_DCOP_INTERFACE_H
#define BlockSelection_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>

namespace KTextEditor
{
	class BlockSelectionInterface;
	/**
	This is the main interface to the @ref BlockSelectionInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref BlockSelectionInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class BlockSelectionDCOPInterface :  virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentBlockSelectionInterface - The parent @ref BlockSelectionInterface object
		that will provide us with the functions for the interface.
		*/
		BlockSelectionDCOPInterface( BlockSelectionInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		*/
		virtual ~BlockSelectionDCOPInterface();
	k_dcop:
	  	uint blockSelectionInterfaceNumber ();
		
		/**
		* is blockselection mode on ?
		* if the blockselection mode is on, the selections
		* applied via the SelectionInterface are handled as
		* blockselections and the paste functions of the
		* ClipboardInterface works blockwise (copy too, but
		* thats clear I hope ;)
		*/
		bool blockSelectionMode ();

		/**
		* set blockselection mode to state "on"
		*/
		bool setBlockSelectionMode (bool on) ;

		/**
		* toggle blockseletion mode
		*/
		bool toggleBlockSelectionMode ();
	
	private:
		BlockSelectionInterface *m_parent;
	};
};
#endif
