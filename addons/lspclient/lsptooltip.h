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
    LspTooltip();
    void show(const QString &text, QPoint pos, KTextEditor::View *v);

private:
    Tooltip *m_tooltip;
};

#endif // LSPTOOLTIP_H
