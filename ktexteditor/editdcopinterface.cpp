#include "editdcopinterface.h"
#include "editinterface.h"
#include "document.h"

#include <dcopclient.h>

EditDCOPInterface::EditDCOPInterface( Document *ParentDocument)
	: DCOPObject(ParentDocument->name())
{
	m_Document = ParentDocument;
}

EditDCOPInterface::~KMainWindowInterface()
{

}

QString EditDCOPInterface::text ()
{
	return m_Document->text();
}

QString EditDCOPInterface::textLine ( int line )
{
	return m_Document->textLine(line);
}

int EditDCOPInterface::numLines ()
{
	return m_Document->numLines();
}

int EditDCOPInterface::length ()
{
	return m_Document->linghth();
}

void EditDCOPInterface::setText ( QString &text )
{
	m_Document->setText(text);
}

bool EditDCOPInterface::insertText ( QString &text, int line, int col )
{
	return m_Document->insertText( text, line, col);
}

bool EditDCOPInterface::removeText ( int line, int col, int len )
{
	return m_Document->removeText( line, col, len);
}

bool EditDCOPInterface::insertLine (QString &text, int line)
{
	return m_Document->insertLine( text, line);
}

bool EditDCOPInterface::removeLine ( int line )
{
	return m_Document->removeLine( line );
}
