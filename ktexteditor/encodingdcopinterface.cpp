#include "encodingdcopinterface.h"
#include "encodinginterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

EncodingDCOPInterface::EncodingDCOPInterface( EncodingInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

EncodingDCOPInterface::~EncodingDCOPInterface()
{

 }
uint EncodingDCOPInterface::encodingInterfaceNumber ()
{
	return m_parent->encodingInterfaceNumber ();
}
void EncodingDCOPInterface::setEncoding (QString e) 
{
	m_parent->setEncoding (e); 
}
QString EncodingDCOPInterface::encoding()
{
	return m_parent->encoding();
}
