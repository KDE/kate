#include "editinterface.h"
#include "editdcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateEditInterface
	{
	public:
		PrivateEditInterface()
		{
		interface = 0;
		}
		~PrivateEditInterface(){}
	// Data Members
	EditDCOPInterface *interface;
	};

};


EditInterface::EditInterface()
{
	d = new PrivateEditInterface();
	EditInterfaceNumber++;
	QString name = "Edit-Interface#" + QString::number(EditInterfaceNumber);
	 d->interface = new EditDCOPInterface(this, name.latin1());
}
EditInterface::~EditInterface()
{
 	delete d->interface;
	delete d;
	EditInterfaceNumber--;
}
