#include "kateprojectclosingcontextmenu.h"

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <KLocalizedString>

void KateProjectClosingContextMenu::exec(QList<KateProject*> projectList, const QPoint &pos, KateProjectPluginView *parent)
{
    /**
     * Create context menu
     */
    QMenu menu;

    /**
     * fill menu
     */
    QAction** actions = new QAction*[projectList.size()];
    for(int i = 0; i<projectList.size(); i++)
        actions[i] = menu.addAction(QIcon::fromTheme(QStringLiteral("window-close")), i18n(projectList[i]->name().toLocal8Bit().data()));

    /**
     * run menu and handle the triggered action
     */
    if (QAction* const action = menu.exec(pos)) 
        for(int i = 0; i<projectList.size(); i++)
            if (action == actions[i])
                parent->projectAboutToClose(projectList[i]);
}


