#include "codecompletioninterface.h"
//#include "editdcopinterface.h"
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