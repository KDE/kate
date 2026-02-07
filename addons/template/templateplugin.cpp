// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "templateplugin.h"

#include <KActionCollection>
#include <KPluginFactory>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLayout>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

using namespace Qt::Literals::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(TemplatePluginFactory, "plugin.json", registerPlugin<TemplatePlugin>();)

TemplatePlugin::TemplatePlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

QObject *TemplatePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new TemplatePluginView(this, mainWindow);
}

TemplatePluginView::TemplatePluginView(TemplatePlugin *, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName(u"templateplugin"_s, i18n("File Templates"));
    setXMLFile(u"ui.rc"_s);

    QAction *a = actionCollection()->addAction(u"new_from_template"_s);
    a->setText(i18n("New From Template"));
    a->setIcon(QIcon::fromTheme(u"document-new-from-template"_s));
    KActionCollection::setDefaultShortcut(a, QKeySequence((Qt::ALT | Qt::SHIFT | Qt::Key_N)));
    connect(a, &QAction::triggered, this, &TemplatePluginView::crateNewFromTemplate);

    m_mainWindow->guiFactory()->addClient(this);
}

TemplatePluginView::~TemplatePluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

void TemplatePluginView::crateNewFromTemplate()
{
    QString currentFolder;
    const auto view = m_mainWindow->activeView();
    if (view && view->document()) {
        QFileInfo info(view->document()->url().path());
        currentFolder = info.absolutePath();
    }

    if (!m_template) {
        m_template = new Template(view);
        connect(m_template, &Template::templateCopied, this, &TemplatePluginView::templateCrated);
    }
    m_template->setOutputFolder(currentFolder);
    m_template->exec();
}

void TemplatePluginView::templateCrated(const QString &fileToOpen)
{
    if (!m_template) {
        qWarning("m_template not created yet!");
        return;
    }
    m_template->hide();
    if (fileToOpen.isEmpty()) {
        return;
    }

    if (QFileInfo(fileToOpen).isFile()) {
        QUrl fileUrl = QUrl::fromLocalFile(fileToOpen);
        m_mainWindow->openUrl(fileUrl);
    } else {
        // Try to open the folder with the project plugin
        QObject *projectPluginView = m_mainWindow->pluginView(u"kateprojectplugin"_s);
        if (projectPluginView) {
            QDir dir(fileToOpen);
            QMetaObject::invokeMethod(projectPluginView, "openDirectoryOrProject", Q_ARG(const QDir &, dir));
        }
    }
}

#include "templateplugin.moc"

#include "moc_templateplugin.cpp"
