/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "kateprojectinfoviewterminal.h"
#include "kateprojectpluginview.h"

#include <klocalizedstring.h>
#include <kde_terminal_interface.h>
#include <KPluginLoader>

KPluginFactory *KateProjectInfoViewTerminal::s_pluginFactory = nullptr;

KateProjectInfoViewTerminal::KateProjectInfoViewTerminal(KateProjectPluginView *pluginView, const QString &directory)
    : QWidget()
    , m_pluginView(pluginView)
    , m_directory(directory)
    , m_konsolePart(nullptr)
{
    /**
     * layout widget
     */
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

KateProjectInfoViewTerminal::~KateProjectInfoViewTerminal()
{
    /**
     * avoid endless loop
     */
    if (m_konsolePart) {
        disconnect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
    }
}

KPluginFactory *KateProjectInfoViewTerminal::pluginFactory()
{
    if (s_pluginFactory) {
        return s_pluginFactory;
    }
    return s_pluginFactory = KPluginLoader(QStringLiteral("konsolepart")).factory();
}

void KateProjectInfoViewTerminal::showEvent(QShowEvent *)
{
    /**
     * we delay the terminal construction until we have some part to have a usable WINDOWID, see bug 411965
     */
    if (!m_konsolePart) {
        loadTerminal();
    }
}

void KateProjectInfoViewTerminal::loadTerminal()
{
    /**
     * null in any case, if loadTerminal fails below and we are in the destroyed event
     */
    m_konsolePart = nullptr;
    setFocusProxy(nullptr);

    /**
     * we shall not arrive here without a factory, if it is not there, no terminal toolview shall be created
     */
    Q_ASSERT(pluginFactory());

    /**
     * create part
     */
    m_konsolePart = pluginFactory()->create<KParts::ReadOnlyPart>(this, this);
    if (!m_konsolePart) {
        return;
    }

    /**
     * init locale translation stuff
     */
    // FIXME KF5 KGlobal::locale()->insertCatalog("konsole");

    /**
     * switch to right directory
     */
    qobject_cast<TerminalInterface *>(m_konsolePart)->showShellInDir(m_directory);

    /**
     * add to widget
     */
    m_layout->addWidget(m_konsolePart->widget());
    setFocusProxy(m_konsolePart->widget());

    /**
     * guard destruction, create new terminal!
     */
    connect(m_konsolePart, &KParts::ReadOnlyPart::destroyed, this, &KateProjectInfoViewTerminal::loadTerminal);
    connect(m_konsolePart, SIGNAL(overrideShortcut(QKeyEvent *, bool &)), this, SLOT(overrideShortcut(QKeyEvent *, bool &)));
}

void KateProjectInfoViewTerminal::overrideShortcut(QKeyEvent *, bool &override)
{
    /**
     * let konsole handle all shortcuts
     */
    override = true;
}
