#ifndef KATE_TAB_MIME_DATA_H
#define KATE_TAB_MIME_DATA_H

#include <QMimeData>

#include "kateviewspace.h"

#include <optional>

namespace KTextEditor
{
class Document;
}

class TabMimeData : public QMimeData
{
    Q_OBJECT
public:
    struct DroppedData {
        int line = -1;
        int col = -1;
        QUrl url;
    };

    TabMimeData(KateViewSpace *vs, KTextEditor::Document *d);

    static bool hasValidData(const QMimeData *md);

    static std::optional<DroppedData> data(const QMimeData *md);

    KateViewSpace *const sourceVS;
    KTextEditor::Document *const doc;
};

#endif
