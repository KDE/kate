#include "searchinterface.h"
#include "searchdcopinterface.h"

using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateSearchInterface
	{
	public:
		PrivateSearchInterface()
		{
		interface = 0;
		}
		~PrivateSearchInterface(){}
	// Data Members
	SearchDCOPInterface *interface;
	};

};

unsigned int SearchInterface::globalSearchInterfaceNumber = 0;

SearchInterface::SearchInterface()
{
	d = new PrivateSearchInterface();
	globalSearchInterfaceNumber++;
	mySearchInterfaceNumber=globalSearchInterfaceNumber;
        QString name = "SearchInterface#" + QString::number(mySearchInterfaceNumber);
	 d->interface = new SearchDCOPInterface(this, name.latin1());
}
SearchInterface::~SearchInterface()
{
  delete d->interface;
  delete d;
}

unsigned int SearchInterface::searchInterfaceNumber () const
{
  return mySearchInterfaceNumber;
}
