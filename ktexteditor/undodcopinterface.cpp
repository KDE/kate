#include "undodcopinterface.h"
#include "undointerface.h"

#include <dcopclient.h>
using namespace KTextEditor;

UndoDCOPInterface::UndoDCOPInterface( UndoInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

UndoDCOPInterface::~UndoDCOPInterface()
{

}

uint UndoDCOPInterface::undoInterfaceNumber ()
{
	return m_parent->undoInterfaceNumber();
}
void UndoDCOPInterface::undo ()
{
	m_parent->undo();
}
void UndoDCOPInterface::redo () 
{
	m_parent->redo();
}
void UndoDCOPInterface::clearUndo () 
{
	m_parent->clearUndo();
}
void UndoDCOPInterface::clearRedo () 
{
	m_parent->clearRedo();
}
uint UndoDCOPInterface::undoCount () 
{
	return m_parent->undoCount();
}
uint UndoDCOPInterface::redoCount ()
{
	return m_parent->redoCount();
}
uint UndoDCOPInterface::undoSteps ()
{
	return m_parent->undoSteps();
}
void UndoDCOPInterface::setUndoSteps ( uint steps )
{
	return m_parent->setUndoSteps(steps);
}
void UndoDCOPInterface::undoChanged ()
{
	m_parent->undoChanged();
}  