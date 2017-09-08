/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "kterustcompletionplugin.h"
#include "kterustcompletionpluginview.h"
#include "kterustcompletionconfigpage.h"

#include <KConfigGroup>
#include <KDirWatch>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDir>

K_PLUGIN_FACTORY_WITH_JSON(KTERustCompletionPluginFactory, "kterustcompletionplugin.json", registerPlugin<KTERustCompletionPlugin>();)

KTERustCompletionPlugin::KTERustCompletionPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent),
    m_completion(this),
    m_rustSrcWatch(nullptr),
    m_configOk(false)
{
    readConfig();
}

KTERustCompletionPlugin::~KTERustCompletionPlugin()
{
}

QObject *KTERustCompletionPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KTERustCompletionPluginView(this, mainWindow);
}

int KTERustCompletionPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *KTERustCompletionPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    return new KTERustCompletionConfigPage(parent, this);
}

KTERustCompletion *KTERustCompletionPlugin::completion()
{
    return &m_completion;
}

QString KTERustCompletionPlugin::racerCmd() const
{
    return m_racerCmd;
}

void KTERustCompletionPlugin::setRacerCmd(const QString &cmd)
{
    if (cmd != m_racerCmd) {
        m_racerCmd = cmd;

        writeConfig();
        updateConfigOk();
    }
}

QUrl KTERustCompletionPlugin::rustSrcPath() const
{
    return m_rustSrcPath;
}

void KTERustCompletionPlugin::setRustSrcPath(const QUrl &path)
{
    if (path != m_rustSrcPath) {
        m_rustSrcPath = path;

        writeConfig();
        updateConfigOk();
    }
}

bool KTERustCompletionPlugin::configOk() const
{
    return m_configOk;
}

void KTERustCompletionPlugin::updateConfigOk()
{
    m_configOk = false;

    if (m_rustSrcPath.isLocalFile()) {
        QString path = m_rustSrcPath.toLocalFile();

        if (QDir(path).exists()) {
            m_configOk = true;

            if (m_rustSrcWatch && !m_rustSrcWatch->contains(path)) {
                delete m_rustSrcWatch;
                m_rustSrcWatch = nullptr;
            }

            if (!m_rustSrcWatch) {
                m_rustSrcWatch = new KDirWatch(this);
                m_rustSrcWatch->addDir(path, KDirWatch::WatchDirOnly);
                connect(m_rustSrcWatch, &KDirWatch::deleted,
                    this, &KTERustCompletionPlugin::updateConfigOk, Qt::UniqueConnection);
            }
        }
    }
}

void KTERustCompletionPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("kterustcompletion"));
    m_racerCmd = config.readEntry(QStringLiteral("racerCmd"), QStringLiteral("racer"));
    m_rustSrcPath = config.readEntry(QStringLiteral("rustSrcPath"),
        QUrl(QStringLiteral("/usr/local/src/rust/src")));

    updateConfigOk();
}

void KTERustCompletionPlugin::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("kterustcompletion"));
    config.writeEntry(QStringLiteral("racerCmd"), m_racerCmd);
    config.writeEntry(QStringLiteral("rustSrcPath"), m_rustSrcPath);
}

#include "kterustcompletionplugin.moc"
