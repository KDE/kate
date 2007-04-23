/* This file is part of the KDE libraries
 * Copyright (C) 2007 Matt Broadstone <mbroadst@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kte_document_binding.h"

#include <ktexteditor/document.h>
#include <kjs/object.h>
#include <kjsembed/object_binding.h>
#include <kjsembed/variant_binding.h>

using namespace KJSEmbed;

namespace KTEDocumentNS
{

START_QOBJECT_METHOD( mimeType, KTextEditor::Document )
    QString mimeType = object->mimeType();
    result = KJS::jsString(mimeType);
END_QOBJECT_METHOD

START_QOBJECT_METHOD( documentName, KTextEditor::Document )
    QString docName = object->documentName();
    result = KJS::jsString(docName);
END_QOBJECT_METHOD

START_QOBJECT_METHOD( encoding, KTextEditor::Document )
    QString encoding = object->encoding();
    result = KJS::jsString(encoding);
END_QOBJECT_METHOD

START_QOBJECT_METHOD( text, KTextEditor::Document )
    QString text = object->text();
    result = KJS::jsString(text);
END_QOBJECT_METHOD

START_QOBJECT_METHOD( isEmpty, KTextEditor::Document )
    bool success = object->isEmpty();
    result = KJS::jsBoolean(success);
END_QOBJECT_METHOD


START_QOBJECT_METHOD( clear, KTextEditor::Document )
    bool success = object->clear();
    result = KJS::jsBoolean(success);
END_QOBJECT_METHOD

}

NO_ENUMS( KTEDocument )

START_METHOD_LUT( KTEDocument )
    {"mimeType", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::mimeType },
    {"documentName", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::documentName },
    {"encoding", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::encoding },
    {"text", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::text },
    {"isEmpty", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::isEmpty },
    {"clear", 0, KJS::DontDelete|KJS::ReadOnly, &KTEDocumentNS::clear }
END_METHOD_LUT

NO_STATICS( KTEDocument )

KJSO_SIMPLE_BINDING_CTOR( KTEDocument, KTextEditor::Document, QObjectBinding )
KJSO_QOBJECT_BIND( KTEDocument, KTextEditor::Document )




