#include "codecompletioninterface.h"
//#include "editdcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateCodeCompletionInterface
	{
	public:
		PrivateCodeCompletionInterface()
		{
//		interface = 0;
		}
		~PrivateCodeCompletionInterface(){}
	// Data Members
	//CodeCompletionDCOPInterface *interface;
	};

};


CodeCompletionInterface::CodeCompletionInterface()
{
	d = new PrivateCodeCompletionInterface();
	CodeCompletionInterfaceNumber++;
	QString name = "CodeCompletion-Interface#" + QString::number(CodeCompletionInterfaceNumber);
	 //d->interface = new CodeCompletionDCOPInterface(this, name.latin1());
}
CodeCompletionInterface::~CodeCompletionInterface()
{
 	//delete d->interface;
  delete d;
}
