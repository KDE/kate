#include "clipboardinterface.h"
#include "clipboarddcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateClipboardInterface
	{
	public:
		PrivateClipboardInterface()
		{
		interface = 0;
		}
		~PrivateClipboardInterface(){}
	// Data Members
	ClipboardDCOPInterface *interface;
	};

};


ClipboardInterface::ClipboardInterface()
{
	d = new PrivateClipboardInterface();
	ClipboardInterfaceNumber++;
	QString name = "Clipboard-Interface#" + QString::number(ClipboardInterfaceNumber);
	d->interface = new ClipboardDCOPInterface(this, name.latin1());
}
ClipboardInterface::~ClipboardInterface()
{
 	delete d->interface;
	delete d;
	ClipboardInterfaceNumber--;
}
