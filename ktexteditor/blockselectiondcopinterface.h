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
	This is the main interface to the BlockSelectionInterface of KTextEditor.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to BlockSelectionInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class KTEXTEDITOR_EXPORT BlockSelectionDCOPInterface :  virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param Parent the parent BlockSelectionInterface object
		that will provide us with the functions for the interface.
		@param name the QObject's name
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
    * Returns the status of the selection mode - true indicates block selection mode is on.
    * If this is true, selections applied via the SelectionInterface are handled as
    * blockselections and the paste functions of the ClipboardInterface works on
    * rectangular blocks of text rather than normal. (copy too, but thats clear I hope ;)
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
}
#endif
