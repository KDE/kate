#include "editdcopinterface.h"
#include "editinterface.h"

#include <dcopclient.h>

EditDCOPInterface::EditDCOPInterface( EditInterface *ParentEditInterface)
	: DCOPObject("EditorInterface")
{
	m_EditInterface = ParentEditInterface;
}

EditDCOPInterface::~KMainWindowInterface()
{

}

QString EditDCOPInterface::text ()
{
	return m_EditInterface->text();
}

QString EditDCOPInterface::textLine ( int line )
{
	return m_EditInterface->textLine(line);
}

int EditDCOPInterface::numLines ()
{
	return m_EditInterface->numLines();
}

int EditDCOPInterface::length ()
{
	return m_EditInterface->linghth();
}

void EditDCOPInterface::setText ( QString &text )
{
	m_EditInterface->setText(text);
}

bool EditDCOPInterface::insertText ( QString &text, int line, int col )
{
	return m_EditInterface->insertText( text, line, col);
}

bool EditDCOPInterface::removeText ( int line, int col, int len )
{
	return m_EditInterface->removeText( line, col, len);
}

bool EditDCOPInterface::insertLine (QString &text, int line)
{
	return m_EditInterface->insertLine( text, line);
}

bool EditDCOPInterface::removeLine ( int line )
{
	return m_EditInterface->removeLine( line );
}
