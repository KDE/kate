#include "selectiondcopinterface.h"
#include "selectioninterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

SelectionDCOPInterface::SelectionDCOPInterface( SelectionInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

SelectionDCOPInterface::~SelectionDCOPInterface()
{

}

    /**
    *  @return set the selection from line_start,col_start to line_end,col_end
    */
     bool SelectionDCOPInterface::setSelection ( uint startLine, uint startCol, uint endLine, uint endCol )
     {
	return m_parent->setSelection ( startLine, startCol, endLine, endCol );
     }

    /**
    *  removes the current Selection (not Text)
    */
     bool SelectionDCOPInterface::clearSelection ()
     {
	return m_parent->clearSelection();
     }

    /**
    *  @return true if there is a selection
    */
     bool SelectionDCOPInterface::hasSelection ()
     {
	return m_parent->hasSelection();
	}

    /**
    *  @return a QString for the selected text
    */
     QString SelectionDCOPInterface::selection ()
     {
	return m_parent->selection();
     }

    /**
    *  removes the selected Text
    */
     bool SelectionDCOPInterface::removeSelectedText ()
     {
	return m_parent->removeSelectedText();
     }

    /**
    *  select the whole text
    */
     bool SelectionDCOPInterface::selectAll ()
     {
	return m_parent->selectAll();
     }
