#include "viewcursordcopinterface.h"
#include "viewcursorinterface.h"

#include <qpoint.h>

#include <dcopclient.h>
using namespace KTextEditor;

ViewCursorDCOPInterface::ViewCursorDCOPInterface( ViewCursorInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

ViewCursorDCOPInterface::~ViewCursorDCOPInterface()
{

}

uint ViewCursorDCOPInterface::viewCursorInterfaceNumber ()
{
	return m_parent->viewCursorInterfaceNumber ();
}

::QPoint ViewCursorDCOPInterface::cursorCoordinates ()
{
	return m_parent->cursorCoordinates ();
}

void ViewCursorDCOPInterface::cursorPosition (uint line, uint col)
{
	m_parent->cursorPosition (&line,  &col);
}

void ViewCursorDCOPInterface::cursorPositionReal (uint line, uint col)
{
	m_parent->cursorPositionReal (&line,  &col);
}

bool ViewCursorDCOPInterface::setCursorPosition (uint line, uint col)
{
	return m_parent->setCursorPosition ( line, col);
}

bool ViewCursorDCOPInterface::setCursorPositionReal (uint line, uint col)
{
	return m_parent->setCursorPositionReal ( line, col);
}

uint ViewCursorDCOPInterface::cursorLine ()
{
	return m_parent->cursorLine ();
}

uint ViewCursorDCOPInterface::cursorColumn ()
{
	return m_parent->cursorColumn ();
}

uint ViewCursorDCOPInterface::cursorColumnReal ()
{
	return m_parent->cursorColumnReal ();
}

void ViewCursorDCOPInterface::cursorPositionChanged ()
{
	 m_parent->cursorPositionChanged ();
}
