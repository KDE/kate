#ifndef SELECTION_DCOP_INTERFACE_H
#define SELECTION_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>

namespace KTextEditor
{
	class SelectionInterface;
	/**
	This is the main interface to the @ref SelectionInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref SelectionInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class SelectionDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentSelectionInterface - The parent @ref SelectionInterface object
		that will provide us with the functions for the interface.
		*/
		SelectionDCOPInterface( SelectionInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		**/
		virtual ~SelectionDCOPInterface();
	k_dcop:
    /**
    *  @return set the selection from line_start,col_start to line_end,col_end
    */
     bool setSelection ( uint startLine, uint startCol, uint endLine, uint endCol );

    /**
    *  removes the current Selection (not Text)
    */
     bool clearSelection ();

    /**
    *  @return true if there is a selection
    */
     bool hasSelection ();

    /**
    *  @return a QString for the selected text
    */
     QString selection ();

    /**
    *  removes the selected Text
    */
     bool removeSelectedText ();

    /**
    *  select the whole text
    */
     bool selectAll ();

	private:
		SelectionInterface *m_parent;
	};
};
#endif


