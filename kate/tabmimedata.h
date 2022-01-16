#ifndef KATE_TAB_MIME_DATA_H
#define KATE_TAB_MIME_DATA_H

#include <QMimeData>

#include "kateviewspace.h"

namespace KTextEditor
{
class Document;
}

class TabMimeData : public QMimeData
{
    Q_OBJECT
public:
    TabMimeData(KateViewSpace *vs, KTextEditor::Document *d, int sourceTabIdx);

    KateViewSpace *const sourceVS;
    KTextEditor::Document *const doc;
    const int tabIdx;
};

#endif
