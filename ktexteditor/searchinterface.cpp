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


SearchInterface::SearchInterface()
{
	d = new PrivateSearchInterface();
	SearchInterfaceNumber++;
	QString name = "Search-Interface#" + QString::number(SearchInterfaceNumber);
	 d->interface = new SearchDCOPInterface(this, name.latin1());
}
SearchInterface::~SearchInterface()
{
 	delete d->interface;
	delete d;
	SearchInterfaceNumber--;
}
