#ifndef EDIT_DCOP_INTERFACE_H
#define EDIT_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>

class EditInterface;
/**
This is the main interface to the @ref EditInterface of KTextEdit.
This will provide a consistant dcop interface to all KDE applications that use it.
@short DCOP interface to @ref EditInterface.
@author Ian Reinhart Geiser <geiseri@yahoo.com>
*/
class EditDCOPInterface : virtual public DCOPObject
{
K_DCOP

public:
	/**
	Construct a new interface object for the text editor.
	@param ParentEditInterface - The parent @ref EditInterface object
	that will provide us with the functions for the interface.
	*/
	EditDCOPInterface( EditInterface *ParentEditInterface );
	/**
	Destructor
	Cleans up the object.
	**/
	~EditDCOPInterface();
k_dcop:
	/**
	* @return the complete document as a single QString
	*/
	virtual QString text ();

	/**
	* @return All the text from the requested line.
	*/
	virtual QString textLine ( int line );

	/**
	* @return The current number of lines in the document
	*/
	virtual int numLines ();

	/**
	* @return the number of characters in the document
	*/
	virtual int length ();

	/**
	* Set the given text into the view.
	* Warning: This will overwrite any data currently held in this view.
	*/
	virtual void setText (QString &text );

	/**
	*  Inserts text at line "line", column "col"
	*  returns true if success
	*/
	virtual bool insertText ( QString &text, int line, int col);

	/**
	*  remove text at line "line", column "col"
	*  returns true if success
	*/
	virtual bool removeText ( int line, int col, int len );

	/**
	* Insert line(s) at the given line number. If the line number is -1
	* (the default) then the line is added to end of the document
	*/
	virtual bool insertLine ( QString &text, int line );

	/**
	* Insert line(s) at the given line number. If the line number is -1
	* (the default) then the line is added to end of the document
	*/
	virtual bool removeLine ( int line );
private:
	EditInterface *m_EditInterface;
};

#endif


