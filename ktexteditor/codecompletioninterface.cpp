
#include "codecompletioninterface.h"
#include "view.h"

using namespace KTextEditor;

namespace KTextEditor
{
  class PrivateCodeCompletionInterface
  {
    public:
      PrivateCodeCompletionInterface() {}
      ~PrivateCodeCompletionInterface(){}

  };
};

unsigned int CodeCompletionInterface::globalCodeCompletionInterfaceNumber = 0;

CodeCompletionInterface::CodeCompletionInterface()
{
  globalCodeCompletionInterfaceNumber++;
  myCodeCompletionInterfaceNumber = globalCodeCompletionInterfaceNumber++;

  d = new PrivateCodeCompletionInterface();
}

CodeCompletionInterface::~CodeCompletionInterface()
{
  delete d;
}

unsigned int CodeCompletionInterface::codeCompletionInterfaceNumber () const
{
  return myCodeCompletionInterfaceNumber;
}

void CodeCompletionInterface::setCodeCompletionInterfaceDCOPSuffix (const QCString &/*suffix*/)
{
  //d->interface->setObjId ("CodeCompletionInterface#"+suffix);
}

CodeCompletionInterface *KTextEditor::codeCompletionInterface (View *view)
{                
  if (!view)
    return 0;

  return static_cast<CodeCompletionInterface*>(view->qt_cast("KTextEditor::CodeCompletionInterface"));
}


