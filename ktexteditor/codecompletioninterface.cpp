
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

CodeCompletionInterface *KTextEditor::codeCompletionInterface (View *view)
{
  return static_cast<CodeCompletionInterface*>(view->qt_cast("KTextEditor::CodeCompletionInterface"));
}


