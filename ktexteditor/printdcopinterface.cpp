#include "printdcopinterface.h"
#include "printinterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

PrintDCOPInterface::PrintDCOPInterface( PrintInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

PrintDCOPInterface::~PrintDCOPInterface()
{

}

uint PrintDCOPInterface::printInterfaceNumber () 
{
	return m_parent->printInterfaceNumber();
}
bool PrintDCOPInterface::printDialog ()
{
	return m_parent->printDialog();
}
bool PrintDCOPInterface::print ()  
{
	return m_parent->print();
}

