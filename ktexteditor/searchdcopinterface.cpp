#include "searchdcopinterface.h"
#include "searchinterface.h"

#include <dcopclient.h>
using namespace KTextEditor;

SearchDCOPInterface::SearchDCOPInterface( SearchInterface *Parent, const char *name)
	: DCOPObject(name)
{
	m_parent = Parent;
}

SearchDCOPInterface::~SearchDCOPInterface()
{

}

bool SearchDCOPInterface::findFirstString( QString text)
{

}
bool SearchDCOPInterface::findNextString( QString text)
{

}
bool SearchDCOPInterface::findPreviousString( QString text)
{

}
bool SearchDCOPInterface::findLastString( QString text)
{

}

bool SearchDCOPInterface::findFirstRegExp( QString regexp)
{

}
bool SearchDCOPInterface::findNextRegExp( QString regexp)
{

}
bool SearchDCOPInterface::findPreviousRegExp( QString regexp)
{

}
bool SearchDCOPInterface::findLastRegExp(const QString regexp)
{

}

int SearchDCOPInterface::currentMatchLine()
{

}
int SearchDCOPInterface::currentMatchCol()
{

}
int SearchDCOPInterface::currentMatchLength()
{

}
