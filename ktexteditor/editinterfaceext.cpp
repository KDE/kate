#include "editinterfaceext.h"
#include "document.h"

using namespace KTextEditor;

uint EditInterfaceExt::globalEditInterfaceExtNumber = 0;

EditInterfaceExt::EditInterfaceExt()
	: d(0L)
{
  globalEditInterfaceExtNumber++;
  myEditInterfaceExtNumber = globalEditInterfaceExtNumber;
}

EditInterfaceExt::~EditInterfaceExt()
{
}

uint EditInterfaceExt::editInterfaceExtNumber() const
{
  return myEditInterfaceExtNumber;
}

EditInterfaceExt *KTextEditor::editInterfaceExt (Document *doc)
{
  if (!doc)
    return 0;

  return static_cast<EditInterfaceExt*>(doc->qt_cast("KTextEditor::EditInterfaceExt"));
}

