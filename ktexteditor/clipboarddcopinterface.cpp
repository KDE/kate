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

    /**
    * Copies selected text.
    */
    void ClipboardDCOPInterface::copy ( )
    {
	m_parent->copy();
    }

    /**
    * Cuts selected text.
    */
    void ClipboardDCOPInterface::cut ( )
    {
	m_parent->cut();
    }

    /**
    * Pastes text from clipboard.
    */
    void ClipboardDCOPInterface::paste ( )
    {
	m_parent->paste();
    }
