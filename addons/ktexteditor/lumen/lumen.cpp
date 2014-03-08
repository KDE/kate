/*
 * Copyright 2014  David Herberth kde@dav1d.de
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
#include <kaboutdata.h>
#include <ktexteditor/document.h>
#include <ktexteditor/range.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/texthintinterface.h>
#include <kactioncollection.h>
#include <QtCore/QFile>
#include <QtCore/QDir>


K_PLUGIN_FACTORY_DEFINITION(
    LumenPluginFactory, registerPlugin<LumenPlugin>("ktexteditor_lumen");
)

K_EXPORT_PLUGIN(
    LumenPluginFactory(
        KAboutData(
            "ktexteditor_lumen",
            "ktexteditor_plugins",
            ki18n("Lumen"),
            "0.1",
            ki18n("Lumen"),
            KAboutData::License_LGPL_V2,
            ki18n("© David Herberth"),
            ki18n("D Autocompletion plugin using DCD as completion server.")
        )
    )
)


LumenPluginView::LumenPluginView(LumenPlugin *plugin, KTextEditor::View *view): QObject(plugin),KXMLGUIClient(view),m_view(view),m_registered(false)
{
    m_plugin = plugin;
    m_model = new LumenCompletionModel((QObject*)m_view, m_plugin->dcd());

    KTextEditor::Document* document = view->document();

    connect(document, SIGNAL(documentUrlChanged(KTextEditor::Document*)),
            this, SLOT(urlChanged(KTextEditor::Document*)));

    registerCompletion();
    registerTextHints();
}

void LumenPluginView::registerCompletion()
{
    KTextEditor::CodeCompletionInterface *completion =
        qobject_cast<KTextEditor::CodeCompletionInterface*>(m_view);

    bool isD = m_view->document()->url().path().endsWith(".d") ||
               m_view->document()->highlightingMode() == "D";

    if (isD && !m_registered) {
        completion->registerCompletionModel(m_model);
        m_registered = true;
    } else if(!isD && m_registered) {
        completion->unregisterCompletionModel(m_model);
        m_registered = false;
    }
}

void LumenPluginView::registerTextHints()
{
    KTextEditor::TextHintInterface *th =
        qobject_cast<KTextEditor::TextHintInterface*>(m_view);
    th->enableTextHints(500);

    connect(m_view, SIGNAL(needTextHint(const KTextEditor::Cursor&, QString &)),
            this, SLOT(getTextHint(const KTextEditor::Cursor&, QString &)));
}

void LumenPluginView::getTextHint(const Cursor& cursor, QString& text)
{
    KTextEditor::Document* document = m_view->document();

    KTextEditor::Cursor cursorEnd = document->documentEnd();
    KTextEditor::Range range0c = KTextEditor::Range(0, 0, cursor.line(), cursor.column());
    KTextEditor::Range rangece = KTextEditor::Range(cursor.line(), cursor.column(),
                                                    cursorEnd.line(), cursorEnd.column());
    QString text0c = document->text(range0c, false);
    QByteArray utf8 = text0c.toUtf8();
    int offset = utf8.length();
    utf8.append(document->text(rangece, false).toUtf8());

    text = m_plugin->dcd()->doc(utf8, offset).trimmed().replace("\\n", "\n");
}

LumenPluginView::~LumenPluginView()
{
}

void LumenPluginView::urlChanged(Document* document)
{
    registerCompletion();

    QStringList paths;
    for (KUrl url = document->url(); !url.equals(KUrl("/")); url = url.upUrl()) {
        url = url.directory();
        url.addPath(".lumenconfig");

        QFile file(url.path());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                QString path = file.readLine().trimmed();
                // KUrl doesn really provide this functionallity
                if (QDir::isRelativePath(path)){
                    path = QDir::cleanPath(
                      url.directory() + QDir::separator() + path
                    );
                }

                paths.append(path);
            }
        }
    }

    if (!paths.isEmpty()) {
        m_plugin->dcd()->addImportPath(paths);
    }
}


LumenPlugin::LumenPlugin(QObject *parent, const QVariantList &): Plugin(parent)
{
    m_dcd = new DCD(9166, "dcd-server", "dcd-client");
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


void LumenPlugin::addView(KTextEditor::View *view)
{
    m_views.insert(view, new LumenPluginView(this, view));
}

void LumenPlugin::removeView(KTextEditor::View *view)
{
    delete m_views.take(view);
}

#include "lumen.moc"
