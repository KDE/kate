#include "editdcopinterface.h"
#include "editinterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

EditDCOPInterface::EditDCOPInterface( EditInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

EditDCOPInterface::~EditDCOPInterface()
{

}

QString EditDCOPInterface::text ()
{
	return m_parent->text();
}

QString EditDCOPInterface::textLine ( uint line )
{
	return m_parent->textLine(line);
}

int EditDCOPInterface::numLines ()
{
	return m_parent->numLines();
}

int EditDCOPInterface::length ()
{
	return m_parent->length();
}

void EditDCOPInterface::setText ( const QString &text )
{
	m_parent->setText(text);
}

bool EditDCOPInterface::insertText ( uint line, uint col, const QString &text )
{
	return m_parent->insertText( line, col, text);
}

bool EditDCOPInterface::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
	return m_parent->removeText( startLine, startCol, endLine, endCol);
}

bool EditDCOPInterface::insertLine ( uint line, const QString &text )
{
	return m_parent->insertLine( line, text);
}

bool EditDCOPInterface::removeLine ( uint line )
{
	return m_parent->removeLine( line );
}
