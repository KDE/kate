#include "wordwrapinterface.h"

using namespace KTextEditor;

namespace KTextEditor
{
class PrivateWordWrapInterface
{
  public:
    PrivateWordWrapInterface() {}
    ~PrivateWordWrapInterface() {}
  };
};

unsigned int WordWrapInterface::globalWordWrapInterfaceNumber = 0;

WordWrapInterface::WordWrapInterface()
{
  globalWordWrapInterfaceNumber++;
  myWordWrapInterfaceNumber = globalWordWrapInterfaceNumber++;

  d = new PrivateWordWrapInterface();
}
WordWrapInterface::~WordWrapInterface()
{
  delete d;
}

unsigned int WordWrapInterface::wordWrapInterfaceNumber () const
{
  return myWordWrapInterfaceNumber;
}
