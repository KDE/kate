#include "viewstatusmsgdcopinterface.h"
#include "viewstatusmsginterface.h"

#include <qstring.h>

#include <dcopclient.h>
using namespace KTextEditor;

ViewStatusMsgDCOPInterface::ViewStatusMsgDCOPInterface( ViewStatusMsgInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

ViewStatusMsgDCOPInterface::~ViewStatusMsgDCOPInterface()
{

}

uint ViewStatusMsgDCOPInterface::viewStatusMsgInterfaceNumber ()
{
	return m_parent->viewStatusMsgInterfaceNumber ();
}
void ViewStatusMsgDCOPInterface::viewStatusMsg (::QString msg) 
{
	m_parent->viewStatusMsg(msg);
}

