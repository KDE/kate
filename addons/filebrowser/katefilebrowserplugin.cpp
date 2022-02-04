/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>
   SPDX-FileCopyrightText: 2009 Dominik Haumann <dhaumann kde org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// BEGIN Includes
#include "katefilebrowserplugin.h"
#include "katefilebrowser.h"
#include "katefilebrowserconfig.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QIcon>
#include <QKeyEvent>
// END Includes

K_PLUGIN_FACTORY_WITH_JSON(KateFileBrowserPluginFactory, "katefilebrowserplugin.json", registerPlugin<KateFileBrowserPlugin>();)

// BEGIN KateFileBrowserPlugin
KateFileBrowserPlugin::KateFileBrowserPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

QObject *KateFileBrowserPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    KateFileBrowserPluginView *view = new KateFileBrowserPluginView(this, mainWindow);
    connect(view, &KateFileBrowserPluginView::destroyed, this, &KateFileBrowserPlugin::viewDestroyed);
    m_views.append(view);
    return view;
}

void KateFileBrowserPlugin::viewDestroyed(QObject *view)
{
    // do not access the view pointer, since it is partially destroyed already
    m_views.removeAll(static_cast<KateFileBrowserPluginView *>(view));
}

int KateFileBrowserPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *KateFileBrowserPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    if (!m_views[0]->m_fileBrowser) {
        m_views[0]->createFileBrowser(true);
    }
    return new KateFileBrowserConfigPage(parent, m_views[0]->m_fileBrowser.get());
}
// END KateFileBrowserPlugin

// BEGIN KateFileBrowserPluginView
KateFileBrowserPluginView::KateFileBrowserPluginView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_toolView(mainWindow->createToolView(plugin,
                                            QStringLiteral("kate_private_plugin_katefileselectorplugin"),
                                            KTextEditor::MainWindow::Left,
                                            QIcon::fromTheme(QStringLiteral("document-open")),
                                            i18n("Filesystem Browser")))
    , m_mainWindow(mainWindow)
{
    m_toolView->installEventFilter(this);
    connect(m_toolView.get(), SIGNAL(toolVisibleChanged(bool)), this, SLOT(createFileBrowser(bool)));
}

void KateFileBrowserPluginView::readSessionConfig(const KConfigGroup &config)
{
    if (m_fileBrowser) {
        m_fileBrowser->readSessionConfig(config);
    } else {
        cg.reset(new KConfigGroup(config));
    }
}

void KateFileBrowserPluginView::writeSessionConfig(KConfigGroup &config)
{
    if (m_fileBrowser) {
        m_fileBrowser->writeSessionConfig(config);
    }
}

void KateFileBrowserPluginView::createFileBrowser(bool visible)
{
    if (!visible) {
        return;
    }

    // we must not have a filebrowser
    Q_ASSERT(!m_fileBrowser);

    disconnect(m_toolView.get(), SIGNAL(toolVisibleChanged(bool)), this, SLOT(createFileBrowser(bool)));
    m_fileBrowser = std::make_unique<KateFileBrowser>(m_mainWindow, m_toolView.get());
    m_fileBrowser->readSessionConfig(*cg);
    // delete the config now
    cg.reset();
}

bool KateFileBrowserPluginView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((obj == m_toolView.get()) && (ke->key() == Qt::Key_Escape)) {
            m_mainWindow->hideToolView(m_toolView.get());
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}
// ENDKateFileBrowserPluginView

#include "katefilebrowserplugin.moc"
