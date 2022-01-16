#include "tabmimedata.h"

TabMimeData::TabMimeData(KateViewSpace *vs, KTextEditor::Document *d, int sourceTabIdx)
    : QMimeData()
    , sourceVS(vs)
    , doc(d)
    , tabIdx(sourceTabIdx)
{
}
