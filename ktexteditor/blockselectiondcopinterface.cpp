#include "blockselectiondcopinterface.h"
#include "blockselectioninterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

BlockSelectionDCOPInterface::BlockSelectionDCOPInterface( BlockSelectionInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

BlockSelectionDCOPInterface::~BlockSelectionDCOPInterface()
{

}

uint BlockSelectionDCOPInterface::blockSelectionInterfaceNumber ()
{
	return m_parent->blockSelectionInterfaceNumber();
}
bool BlockSelectionDCOPInterface::blockSelectionMode ()
{
	return m_parent->blockSelectionMode ();
}
bool BlockSelectionDCOPInterface::setBlockSelectionMode (bool on) 
{
	return m_parent->setBlockSelectionMode (on);
}
bool BlockSelectionDCOPInterface::toggleBlockSelectionMode ()
{
	return m_parent->toggleBlockSelectionMode ();
}
