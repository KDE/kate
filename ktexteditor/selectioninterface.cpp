#include "selectioninterface.h"
#include "selectiondcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateSelectionInterface
	{
	public:
		PrivateSelectionInterface()
		{
		interface = 0;
		}
		~PrivateSelectionInterface(){}
	// Data Members
	SelectionDCOPInterface *interface;
	};

};


SelectionInterface::SelectionInterface()
{
	d = new PrivateSelectionInterface();
	SelectionInterfaceNumber++;
	QString name = "Selection-Interface#" + QString::number(SelectionInterfaceNumber);
	 d->interface = new SelectionDCOPInterface(this, name.latin1());
}
SelectionInterface::~SelectionInterface()
{
 	delete d->interface;
	delete d;
	SelectionInterfaceNumber--;
}
