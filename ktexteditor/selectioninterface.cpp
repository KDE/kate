#include "selectioninterface.h"
#include "selectiondcopinterface.h"
#include "document.h"

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

unsigned int SelectionInterface::globalSelectionInterfaceNumber = 0;

SelectionInterface::SelectionInterface()
{
	d = new PrivateSelectionInterface();
	globalSelectionInterfaceNumber++;
        mySelectionInterfaceNumber = globalSelectionInterfaceNumber;
	QString name = "SelectionInterface#" + QString::number(mySelectionInterfaceNumber);
	 d->interface = new SelectionDCOPInterface(this, name.latin1());
}
SelectionInterface::~SelectionInterface()
{
  delete d->interface;
  delete d;
}

unsigned int SelectionInterface::selectionInterfaceNumber () const
{
  return mySelectionInterfaceNumber;
}

SelectionInterface *KTextEditor::selectionInterface (Document *doc)
{
  return static_cast<SelectionInterface*>(doc->qt_cast("KTextEditor::SelectionInterface"));
}
