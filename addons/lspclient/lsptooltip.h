#ifndef LSPTOOLTIP_H
#define LSPTOOLTIP_H

#include <QPoint>

class QWidget;
class QString;
class Tooltip;

namespace KTextEditor
{
class View;
}

class LspTooltip
{
public:
    static void show(const QString &text, QPoint pos, KTextEditor::View *v);
};

#endif // LSPTOOLTIP_H
