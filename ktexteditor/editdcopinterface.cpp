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

QString EditDCOPInterface::textLine ( unsigned int line )
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

bool EditDCOPInterface::insertText ( unsigned int line, unsigned int col, const QString &text )
{
	return m_parent->insertText( line, col, text);
}

bool EditDCOPInterface::removeText ( unsigned int startLine, unsigned int startCol, unsigned int endLine, unsigned int endCol )
{
	return m_parent->removeText( startLine, startCol, endLine, endCol);
}

bool EditDCOPInterface::insertLine ( unsigned int line, const QString &text )
{
	return m_parent->insertLine( line, text);
}

bool EditDCOPInterface::removeLine ( unsigned int line )
{
	return m_parent->removeLine( line );
}
