#include "clipboarddcopinterface.h"
#include "clipboardinterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

ClipboardDCOPInterface::ClipboardDCOPInterface( ClipboardInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

ClipboardDCOPInterface::~ClipboardDCOPInterface()
{

}

    void ClipboardDCOPInterface::copy ( )
    {
	m_parent->copy();
    }

    /**
    * copies selected text
    */
    void ClipboardDCOPInterface::cut ( )
    {
	m_parent->cut();
    }

    /**
    * copies selected text
    */
    void ClipboardDCOPInterface::paste ( )
    {
	m_parent->paste();
    }
