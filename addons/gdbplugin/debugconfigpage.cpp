/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2023 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: MIT
*/

#include "debugconfigpage.h"
#include "plugin_kategdb.h"
#include "ui_debugconfig.h"

#include <KLocalizedString>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>

#include <KTextEditor/Editor>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
#include <QPalette>

#include <ktexteditor_utils.h>

DebugConfigPage::DebugConfigPage(QWidget *parent, KatePluginGDB *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::DebugConfigWidget();
    ui->setupUi(this);

    // Fix-up our two text edits to be proper JSON file editors
    updateHighlighters();

    // Ensure we update the highlighters if the repository is updated or theme is changed
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::repositoryReloaded, this, &DebugConfigPage::updateHighlighters);
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &DebugConfigPage::updateHighlighters);

    // Setup default JSON settings
    QFile defaultConfigFile(QStringLiteral(":/debugger/dap.json"));
    if (!defaultConfigFile.open(QIODevice::ReadOnly)) {
        Q_UNREACHABLE();
    }
    Q_ASSERT(defaultConfigFile.isOpen());
    ui->defaultConfig->setPlainText(QString::fromUtf8(defaultConfigFile.readAll()));

    // Setup default config path as placeholder to show user where it is
    ui->edtConfigPath->setPlaceholderText(m_plugin->m_defaultConfigPath.toLocalFile());

    reset();

    connect(ui->edtConfigPath, &KUrlRequester::textChanged, this, &DebugConfigPage::configUrlChanged);
    connect(ui->edtConfigPath, &KUrlRequester::urlSelected, this, &DebugConfigPage::configUrlChanged);

    auto cfgh = [this](int position, int added, int removed) {
        Q_UNUSED(position);
        // Discard format change
        // (e.g. due to syntax highlighting)
        if (added || removed) {
            configTextChanged();
        }
    };
    connect(ui->userConfig->document(), &QTextDocument::contentsChange, this, cfgh);
}

DebugConfigPage::~DebugConfigPage()
{
    delete ui;
}

QString DebugConfigPage::name() const
{
    return QString(i18n("Debugger"));
}

QString DebugConfigPage::fullName() const
{
    return QString(i18n("Debugger"));
}

QIcon DebugConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("debug-run"));
}

void DebugConfigPage::apply()
{
    m_plugin->m_configPath = ui->edtConfigPath->url();
    // Own scope to ensure file is flushed before we signal below in writeConfig!
    {
        QFile configFile(m_plugin->configPath().toLocalFile());
        if (!configFile.open(QIODevice::WriteOnly)) {
            Utils::showMessage(i18n("Failed to open file: %1", m_plugin->m_configPath.toLocalFile()),
                               QIcon(),
                               QStringLiteral("DebugConfig"),
                               MessageType::Error);
        }
        if (configFile.isOpen()) {
            configFile.write(ui->userConfig->toPlainText().toUtf8());
        }
    }

    m_plugin->writeConfig();
}

void DebugConfigPage::reset()
{
    ui->edtConfigPath->setUrl(m_plugin->m_configPath);
    readUserConfig(m_plugin->configPath().toLocalFile());
}

void DebugConfigPage::defaults()
{
    reset();
}

void DebugConfigPage::readUserConfig(const QString &fileName)
{
    QFile configFile(fileName);
    if (configFile.open(QIODevice::ReadOnly)) {
        ui->userConfig->setPlainText(QString::fromUtf8(configFile.readAll()));
    } else {
        ui->userConfig->clear();
    }

    updateConfigTextErrorState();
}

void DebugConfigPage::updateConfigTextErrorState()
{
    const auto userConfigJsonTxt = ui->userConfig->toPlainText().toUtf8();
    if (userConfigJsonTxt.isEmpty()) {
        ui->userConfigError->setText(i18n("No JSON data to validate."));
        return;
    }

    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(userConfigJsonTxt, &error);
    if (error.error == QJsonParseError::NoError) {
        if (json.isObject()) {
            ui->userConfigError->setText(i18n("JSON data is valid."));
        } else {
            ui->userConfigError->setText(i18n("JSON data is invalid: no JSON object"));
        }
    } else {
        ui->userConfigError->setText(i18n("JSON data is invalid: %1", error.errorString()));
    }
}

void DebugConfigPage::configTextChanged()
{
    updateConfigTextErrorState();
    changed();
}

void DebugConfigPage::configUrlChanged()
{
    readUserConfig(ui->edtConfigPath->url().isEmpty() ? m_plugin->m_defaultConfigPath.toLocalFile() : ui->edtConfigPath->url().toLocalFile());
    changed();
}

void DebugConfigPage::updateHighlighters()
{
    for (auto textEdit : {ui->userConfig, ui->defaultConfig}) {
        auto highlighter = new KSyntaxHighlighting::SyntaxHighlighter(textEdit->document());
        highlighter->setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(QStringLiteral("dap.json")));

        // Use monospace font
        textEdit->setFont(KTextEditor::Editor::instance()->font());

        // Use right color scheme
        const auto theme = KTextEditor::Editor::instance()->theme();
        auto pal = qApp->palette();
        pal.setColor(QPalette::Base, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)));
        pal.setColor(QPalette::Highlight, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection)));
        textEdit->setPalette(pal);
        highlighter->setTheme(theme);
        highlighter->rehighlight();
    }
}
