#ifndef KATEPROJECTCLOSINGCONTEXTMENU_H
#define KATEPROJECTCLOSINGCONTEXTMENU_H

#include <QObject>
#include <QString>
#include "kateproject.h"
#include "kateprojectpluginview.h"

/**
 * @todo write docs
 */
class KateProjectClosingContextMenu : public QObject
{
    Q_OBJECT

public:
    
    /**
     * Creating project closing menu from Qlist<KateProject*>
     */
    void exec(QList<KateProject*> projectList, const QPoint &pos, KateProjectPluginView *parent);

};

#endif // KATEPROJECTCLOSINGCONTEXTMENU_H
