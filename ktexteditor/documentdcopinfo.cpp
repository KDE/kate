#include "documentinfo.h"
#include "documentdcopinfo.h"

#include <dcopclient.h>
using namespace KTextEditor;

DocumentInfoDCOPInterface::DocumentInfoDCOPInterface( DocumentInfoInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

DocumentInfoDCOPInterface::~DocumentInfoDCOPInterface()
{

}

QString DocumentInfoDCOPInterface::mimeType()
{
	return m_parent->mimeType();
}
long  DocumentInfoDCOPInterface::fileSize()
{
	return m_parent->fileSize();
}
QString DocumentInfoDCOPInterface::niceFileSize()
{
	return m_parent->niceFileSize();
}
uint DocumentInfoDCOPInterface::documentInfoInterfaceNumber ()
{
	return m_parent->documentInfoInterfaceNumber ();
}
