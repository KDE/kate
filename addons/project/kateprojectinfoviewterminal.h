/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_PROJECT_INFO_VIEW_TERMINAL_H
#define KATE_PROJECT_INFO_VIEW_TERMINAL_H

#include "kateproject.h"

#include <QVBoxLayout>
#include <QKeyEvent>

#include <kparts/part.h>

class KateProjectPluginView;

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectInfoViewTerminal : public QWidget
{
    Q_OBJECT

public:
    /**
     * construct project info view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectInfoViewTerminal(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct info view
     */
    ~KateProjectInfoViewTerminal() override;

    /**
     * our project.
     * @return project
     */
    KateProject *project() const {
        return m_project;
    }

private Q_SLOTS:
    /**
     * Construct a new terminal for this view
     */
    void loadTerminal();

    /**
     * Handle that shortcuts are not eaten by console
     */
    void overrideShortcut(QKeyEvent *event, bool &override);

private:
    /**
     * our plugin view
     */
    KateProjectPluginView *m_pluginView;

    /**
     * our project
     */
    KateProject *m_project;

    /**
     * our layout
     */
    QVBoxLayout *m_layout;

    /**
     * konsole part
     */
    KParts::ReadOnlyPart *m_konsolePart;
};

#endif

