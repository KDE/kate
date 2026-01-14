/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FormatPlugin.h"

#include "CursorPositionRestorer.h"
#include "FormatApply.h"
#include "FormatConfig.h"
#include "Formatters.h"
#include <json_utils.h>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/DocumentCursor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QDir>

K_PLUGIN_FACTORY_WITH_JSON(FormatPluginFactory, "FormatPlugin.json", registerPlugin<FormatPlugin>();)

static QJsonDocument readDefaultConfig()
{
    QFile defaultConfigFile(QStringLiteral(":/formatting/FormatterSettings.json"));
    if (!defaultConfigFile.open(QIODevice::ReadOnly)) {
        Q_UNREACHABLE();
    }
    Q_ASSERT(defaultConfigFile.isOpen());
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(defaultConfigFile.readAll(), &err);
    Q_ASSERT(err.error == QJsonParseError::NoError);
    return doc;
}

FormatPlugin::FormatPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_defaultConfig(readDefaultConfig())
{
    readConfig();
}

QObject *FormatPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new FormatPluginView(this, mainWindow);
}

KTextEditor::ConfigPage *FormatPlugin::configPage(int number, QWidget *parent)
{
    if (number == 0) {
        return new FormatConfigPage(this, parent);
    }
    return nullptr;
}

void FormatPlugin::readConfig()
{
    QString settingsPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/formatting"));
    QDir().mkpath(settingsPath);
    readJsonConfig();

    formatOnSave = m_formatterConfig.value(QStringLiteral("formatOnSave")).toBool(true);
}

void FormatPlugin::readJsonConfig()
{
    QJsonDocument userConfig;
    const QString path = userConfigPath();
    if (QFile::exists(path)) {
        QFile f(path);
        if (f.open(QFile::ReadOnly)) {
            QJsonParseError err;
            const QByteArray text = f.readAll();
            if (!text.isEmpty()) {
                userConfig = QJsonDocument::fromJson(text, &err);
                if (err.error != QJsonParseError::NoError) {
                    QMetaObject::invokeMethod(
                        this,
                        [err] {
                            Utils::showMessage(i18n("Failed to read settings.json. Error: %1", err.errorString()), QIcon(), i18n("Format"), MessageType::Error);
                        },
                        Qt::QueuedConnection);
                }
            }
        }
    }

    if (!userConfig.isEmpty()) {
        m_formatterConfig = json::merge(m_defaultConfig.object(), userConfig.object());
    } else {
        m_formatterConfig = m_defaultConfig.object();
    }
}

QString FormatPlugin::userConfigPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).append(QStringLiteral("/formatting/settings.json"));
}

QJsonObject FormatPlugin::formatterConfig() const
{
    return m_formatterConfig;
}

FormatPluginView::FormatPluginView(FormatPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(plugin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
{
    KXMLGUIClient::setComponentName(QStringLiteral("formatplugin"), i18n("Formatting"));

    connect(m_plugin, &FormatPlugin::configChanged, this, &FormatPluginView::onConfigChanged);

    auto ac = actionCollection();
    auto a = ac->addAction(QStringLiteral("format_document"), this, &FormatPluginView::format);
    a->setText(i18n("Format Document"));
    connect(mainWin, &KTextEditor::MainWindow::viewChanged, this, &FormatPluginView::onActiveViewChanged);

    setXMLFile(QStringLiteral("ui.rc"));

    a = actionCollection()->addAction(QStringLiteral("format_on_save"), this, [this](bool b) {
        m_plugin->formatOnSave = b;
        onActiveViewChanged(nullptr);
        onActiveViewChanged(m_mainWindow->activeView());
    });
    a->setText(i18n("Format on Save"));
    a->setCheckable(true);
    a->setChecked(formatOnSave());
    a->setToolTip(i18n("Disable formatting on save without persisting it in settings"));

    m_mainWindow->guiFactory()->addClient(this);
}

FormatPluginView::~FormatPluginView()
{
    disconnect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &FormatPluginView::onActiveViewChanged);
    m_mainWindow->guiFactory()->removeClient(this);
}

void FormatPluginView::onConfigChanged()
{
    m_lastChecksum = {};
    m_lastMergedConfig = {};
    onActiveViewChanged(nullptr);
    onActiveViewChanged(m_mainWindow->activeView());
}

void FormatPluginView::onActiveViewChanged(KTextEditor::View *v)
{
    if (!v || !v->document()) {
        if (m_activeDoc) {
            disconnect(m_activeDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &FormatPluginView::runFormatOnSave);
        }
        m_activeDoc = nullptr;
        return;
    }

    if (v->document() == m_activeDoc) {
        return;
    }

    if (m_activeDoc) {
        disconnect(m_activeDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &FormatPluginView::runFormatOnSave);
    }

    m_activeDoc = v->document();
    m_lastChecksum = {};

    if (formatOnSave()) {
        connect(m_activeDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &FormatPluginView::runFormatOnSave, Qt::QueuedConnection);
    }
}

void FormatPluginView::runFormatOnSave()
{
    m_triggeredOnSave = true;
    format();
    m_triggeredOnSave = false;
}

void FormatPluginView::format()
{
    if (!m_activeDoc) {
        return;
    }

    // save if modified
    if (m_activeDoc->isModified() && !m_activeDoc->url().toLocalFile().isEmpty()) {
        saveDocument(m_activeDoc);
    }

    if (!m_lastChecksum.isEmpty() && m_activeDoc->checksum() == m_lastChecksum) {
        return;
    }

    const QVariant projectConfig = Utils::projectMapForDocument(m_activeDoc).value(QStringLiteral("formatting"));
    if (projectConfig != m_lastProjectConfig) {
        m_lastProjectConfig = projectConfig;
        if (projectConfig.isValid()) {
            const QJsonObject formattingConfig = QJsonDocument::fromVariant(projectConfig).object();
            if (!formattingConfig.isEmpty()) {
                m_lastMergedConfig = json::merge(m_plugin->formatterConfig(), formattingConfig);
            }
        } else {
            // clear otherwise
            m_lastMergedConfig = QJsonObject();
        }
    }

    if (m_lastMergedConfig.isEmpty()) {
        m_lastMergedConfig = m_plugin->formatterConfig();
    }

    FormatterRunner *formatter = formatterForDoc(m_activeDoc, m_lastMergedConfig);
    if (!formatter) {
        return;
    }

    if (m_triggeredOnSave && !formatter->formatOnSaveEnabled(formatOnSave())) {
        delete formatter;
        return;
    }

    if (m_activeDoc == m_mainWindow->activeView()->document()) {
        formatter->setCursorPosition(m_mainWindow->activeView()->cursorPosition());
    }

    connect(formatter, &FormatterRunner::textFormatted, this, &FormatPluginView::onFormattedTextReceived);
    connect(formatter, &FormatterRunner::message, this, [formatter](const QString &error, MessageType msgType) {
        static QSet<QString> errors;
        if (!errors.contains(error)) {
            formatter->deleteLater();
            const QString msg = formatter->cmdline() + QStringLiteral("\n") + error;
            Utils::showMessage(msg, {}, i18n("Format"), msgType);
            errors.insert(error);
        }
    });
    formatter->run(m_activeDoc);
}

void FormatPluginView::onFormattedTextReceived(FormatterRunner *formatter, KTextEditor::Document *doc, const QByteArray &formattedText, int offset)
{
    formatter->deleteLater();
    if (!doc) {
        qWarning("%s invalid null doc", Q_FUNC_INFO);
        return;
    }

    // Not formatted => no work to do
    if (formattedText.isEmpty()) {
        return;
    }

    // if the user typed something while the formatter ran, ignore the formatted text
    if (formatter->originalText != doc->text()) {
        // qDebug() << "text changed, ignoring format" << doc->documentName();
        return;
    }

    auto setCursorPositionFromOffset = [this, offset, doc] {
        if (offset > -1 && m_mainWindow->activeView()->document() == doc) {
            m_mainWindow->activeView()->setCursorPosition(doc->offsetToCursor(offset));
        }
    };

    // non local file or untitled?
    if (doc->url().toLocalFile().isEmpty()) {
        doc->setText(QString::fromUtf8(formattedText));
        if (m_activeDoc == doc && !m_lastChecksum.isEmpty()) {
            m_lastChecksum.clear();
        }
        setCursorPositionFromOffset();
        return;
    }

    // get a git diff
    const QString patch = diff(doc, formattedText);
    // no difference? => do nothing
    if (patch.isEmpty()) {
        return;
    }

    // create applicable edits
    const std::vector<PatchLine> edits = parseDiff(doc, patch);
    // If the edits are too numerous, just do "setText" as it can be very slow
    if ((int)edits.size() >= (doc->lines() / 2) && doc->lines() > 1000) {
        for (const PatchLine &p : edits) {
            delete p.pos;
        }
        doc->setText(QString::fromUtf8(formattedText));
        saveDocument(doc);
        if (m_activeDoc == doc) {
            m_lastChecksum = doc->checksum();
        }
        setCursorPositionFromOffset();
        return;
    }

    onFormattedPatchReceived(doc, edits, offset == -1);
    setCursorPositionFromOffset();
}

void FormatPluginView::onFormattedPatchReceived(KTextEditor::Document *doc, const std::vector<PatchLine> &patch, bool setCursor)
{
    // Helper to restore cursor position for all views of doc
    CursorPositionRestorer restorer(setCursor ? doc : nullptr);

    applyPatch(doc, patch);
    // finally save the document
    saveDocument(doc);
    if (m_activeDoc == doc) {
        m_lastChecksum = doc->checksum();
    }

    if (setCursor) {
        restorer.restore();
    }
}

void FormatPluginView::saveDocument(KTextEditor::Document *doc)
{
    if (doc->url().isValid() && doc->isModified()) {
        if (formatOnSave() && doc == m_activeDoc) {
            disconnect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, &FormatPluginView::runFormatOnSave);
        }

        doc->documentSave();

        if (formatOnSave() && doc == m_activeDoc) {
            connect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, &FormatPluginView::runFormatOnSave, Qt::QueuedConnection);
        }
    }
}

#include "FormatPlugin.moc"
#include "moc_FormatPlugin.cpp"
