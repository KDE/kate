/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_PROJECT_INFO_VIEW_TERMINAL_H
#define KATE_PROJECT_INFO_VIEW_TERMINAL_H

#include "kateproject.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include <KPluginFactory>
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
     * @param directory base directory for this terminal view
     */
    KateProjectInfoViewTerminal(KateProjectPluginView *pluginView, const QString &directory);

    /**
     * deconstruct info view
     */
    ~KateProjectInfoViewTerminal() override;

    /**
     * global plugin factory to create terminal
     * exposed to allow to skip terminal toolview creation if not possible
     * @return plugin factory for terminal or nullptr if no terminal part there
     */
    static KPluginFactory *pluginFactory();

private Q_SLOTS:
    /**
     * Construct a new terminal for this view
     */
    void loadTerminal();

    /**
     * Handle that shortcuts are not eaten by console
     */
    void overrideShortcut(QKeyEvent *event, bool &override);

protected:
    /**
     * the konsole get shown
     * @param ev show event
     */
    void showEvent(QShowEvent *ev) override;

private:
    /**
     * plugin factory for the terminal
     */
    static KPluginFactory *s_pluginFactory;

    /**
     * our plugin view
     */
    KateProjectPluginView *const m_pluginView;

    /**
     * our start directory for the terminal
     */
    const QString m_directory;

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
