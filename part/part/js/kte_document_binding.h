#ifndef KTE_DOCUMENT_BINDING_H
#define KTE_DOCUMENT_BINDING_H

#include <kjsembed/qobject_binding.h>

namespace KTextEditor
{
  class Document;
}

KJSO_BINDING( KTEDocument, KTextEditor::Document, KJSEmbed::QObjectBinding )

#endif


