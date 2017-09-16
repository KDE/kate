/*
 * Copyright 2014-2015  David Herberth kde@dav1d.de
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "lumen.h"

#include <kpluginfactory.h>
#include <KTextEditor/Document>
#include <KTextEditor/Range>
#include <KTextEditor/View>
#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/MainWindow>
#include <kactioncollection.h>
#include <QtCore/QFile>
#include <QtCore/QDir>


K_PLUGIN_FACTORY_WITH_JSON(LumenPluginFactory, "ktexteditor_lumen.json", registerPlugin<LumenPlugin>();)


LumenPluginView::LumenPluginView(LumenPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWin(mainWin)
    , m_registered(false)
{
    m_model = new LumenCompletionModel((QObject*)m_mainWin, m_plugin->dcd());
    m_hinter = new LumenHintProvider(m_plugin);

    connect(m_mainWin, &KTextEditor::MainWindow::viewCreated, this, &LumenPluginView::viewCreated);

    foreach(KTextEditor::View *view, m_mainWin->views()) {
        viewCreated(view);
    }
}

void LumenPluginView::viewCreated(KTextEditor::View *view)
{
    connect(view->document(), &KTextEditor::Document::documentUrlChanged,
        this, &LumenPluginView::documentChanged,
        Qt::UniqueConnection);
    connect(view->document(), &KTextEditor::Document::highlightingModeChanged,
        this, &LumenPluginView::documentChanged,
        Qt::UniqueConnection);

    connect(view->document(), &Document::documentUrlChanged, this, &LumenPluginView::urlChanged);
    registerCompletion(view);
}

void LumenPluginView::viewDestroyed(QObject *view)
{
    m_completionViews.remove(static_cast<KTextEditor::View *>(view));
}

void LumenPluginView::documentChanged(KTextEditor::Document *document)
{
    foreach(KTextEditor::View *view, document->views()) {
        registerCompletion(view);
        registerTextHints(view);

    }
}

void LumenPluginView::registerCompletion(KTextEditor::View *view)
{
    KTextEditor::CodeCompletionInterface *completion =
        qobject_cast<KTextEditor::CodeCompletionInterface*>(view);

    bool isD = view->document()->url().path().endsWith(QStringLiteral(".d")) ||
               view->document()->highlightingMode() == QStringLiteral("D");

    if (isD && !m_registered) {
        completion->registerCompletionModel(m_model);
        m_registered = true;
    } else if(!isD && m_registered) {
        completion->unregisterCompletionModel(m_model);
        m_registered = false;
    }
}

void LumenPluginView::registerTextHints(KTextEditor::View *view)
{
    KTextEditor::TextHintInterface *th =
        qobject_cast<KTextEditor::TextHintInterface*>(view);

    if (th) {
        th->setTextHintDelay(500);
        th->registerTextHintProvider(m_hinter);
    }
}

LumenPluginView::~LumenPluginView()
{
}

void LumenPluginView::urlChanged(Document* document)
{
    QStringList paths;
    QDir dir = QDir(document->url().toLocalFile());

    for (; !dir.isRoot(); dir.cdUp()) {
        QFile file(dir.filePath(QStringLiteral(".lumenconfig")));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                QString path = QString::fromUtf8(file.readLine().trimmed());

                if (QDir::isRelativePath(path)){
                    path = QDir::cleanPath(dir.filePath(path));
                }

                paths.append(path);
            }
        }
    }

    if (!paths.isEmpty()) {
        m_plugin->dcd()->addImportPath(paths);
    }
}


LumenHintProvider::LumenHintProvider(LumenPlugin* plugin)
    : m_plugin(plugin)
{
}

QString LumenHintProvider::textHint(View* view, const Cursor& position)
{
    KTextEditor::Document* document = view->document();

    KTextEditor::Cursor cursorEnd = document->documentEnd();
    KTextEditor::Range range0c = KTextEditor::Range(0, 0, position.line(), position.column());
    KTextEditor::Range rangece = KTextEditor::Range(position.line(), position.column(),
                                                    cursorEnd.line(), cursorEnd.column());
    QString text0c = document->text(range0c, false);
    QByteArray utf8 = text0c.toUtf8();
    int offset = utf8.length();
    utf8.append(document->text(rangece, false).toUtf8());

    return m_plugin->dcd()->doc(utf8, offset).trimmed().replace(QStringLiteral("\\n"), QStringLiteral("\n"));
}


LumenPlugin::LumenPlugin(QObject *parent, const QList<QVariant> &)
    : Plugin(parent)
{
    m_dcd = new DCD(9166, QStringLiteral("dcd-server"), QStringLiteral("dcd-client"));
    m_dcd->startServer();
}

LumenPlugin::~LumenPlugin()
{
    m_dcd->stopServer();
    delete m_dcd;
}

DCD* LumenPlugin::dcd()
{
    return m_dcd;
}

QObject* LumenPlugin::createView(KTextEditor::MainWindow *mainWin)
{
    return new LumenPluginView(this, mainWin);
}

#include "lumen.moc"
