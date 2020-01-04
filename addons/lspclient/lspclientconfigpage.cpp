/*  SPDX-License-Identifier: MIT

    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "lspclientconfigpage.h"
#include "lspclientplugin.h"
#include "ui_lspconfigwidget.h"

#include <KLocalizedString>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>

#include <QPalette>

LSPClientConfigPage::LSPClientConfigPage(QWidget *parent, LSPClientPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::LspConfigWidget();
    ui->setupUi(this);

    // fix-up our two text edits to be proper JSON file editors
    for (auto textEdit : {ui->userConfig, static_cast<QTextEdit *>(ui->defaultConfig)}) {
        // setup JSON highlighter for the default json stuff
        auto highlighter = new KSyntaxHighlighting::SyntaxHighlighter(textEdit->document());
        highlighter->setDefinition(m_repository.definitionForFileName(QStringLiteral("settings.json")));

        // we want mono-spaced font
        textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

        // we want to have the proper theme for the current palette
        const auto theme = (palette().color(QPalette::Base).lightness() < 128) ? m_repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme) : m_repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
        auto pal = qApp->palette();
        if (theme.isValid()) {
            pal.setColor(QPalette::Base, theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
            pal.setColor(QPalette::Highlight, theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
        }
        textEdit->setPalette(pal);
        highlighter->setTheme(theme);
    }

    // setup default json settings
    QFile defaultConfigFile(QStringLiteral(":/lspclient/settings.json"));
    defaultConfigFile.open(QIODevice::ReadOnly);
    Q_ASSERT(defaultConfigFile.isOpen());
    ui->defaultConfig->setPlainText(QString::fromUtf8(defaultConfigFile.readAll()));

    // setup default config path as placeholder to show user where it is
    ui->edtConfigPath->setPlaceholderText(m_plugin->m_defaultConfigPath.toLocalFile());

    reset();

    for (const auto &cb : {ui->chkSymbolDetails,
                           ui->chkSymbolExpand,
                           ui->chkSymbolSort,
                           ui->chkSymbolTree,
                           ui->chkComplDoc,
                           ui->chkRefDeclaration,
                           ui->chkDiagnostics,
                           ui->chkDiagnosticsMark,
                           ui->chkOnTypeFormatting,
                           ui->chkIncrementalSync,
                           ui->chkSemanticHighlighting,
                           ui->chkAutoHover})
        connect(cb, &QCheckBox::toggled, this, &LSPClientConfigPage::changed);
    connect(ui->edtConfigPath, &KUrlRequester::textChanged, this, &LSPClientConfigPage::configUrlChanged);
    connect(ui->edtConfigPath, &KUrlRequester::urlSelected, this, &LSPClientConfigPage::configUrlChanged);
    connect(ui->userConfig, &QTextEdit::textChanged, this, &LSPClientConfigPage::changed);

    // custom control logic
    auto h = [this]() {
        bool enabled = ui->chkDiagnostics->isChecked();
        ui->chkDiagnosticsHighlight->setEnabled(enabled);
        ui->chkDiagnosticsMark->setEnabled(enabled);
    };
    connect(this, &LSPClientConfigPage::changed, this, h);
}

LSPClientConfigPage::~LSPClientConfigPage()
{
    delete ui;
}

QString LSPClientConfigPage::name() const
{
    return QString(i18n("LSP Client"));
}

QString LSPClientConfigPage::fullName() const
{
    return QString(i18n("LSP Client"));
}

QIcon LSPClientConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("code-context"));
}

void LSPClientConfigPage::apply()
{
    m_plugin->m_symbolDetails = ui->chkSymbolDetails->isChecked();
    m_plugin->m_symbolTree = ui->chkSymbolTree->isChecked();
    m_plugin->m_symbolExpand = ui->chkSymbolExpand->isChecked();
    m_plugin->m_symbolSort = ui->chkSymbolSort->isChecked();

    m_plugin->m_complDoc = ui->chkComplDoc->isChecked();
    m_plugin->m_refDeclaration = ui->chkRefDeclaration->isChecked();

    m_plugin->m_diagnostics = ui->chkDiagnostics->isChecked();
    m_plugin->m_diagnosticsHighlight = ui->chkDiagnosticsHighlight->isChecked();
    m_plugin->m_diagnosticsMark = ui->chkDiagnosticsMark->isChecked();

    m_plugin->m_autoHover = ui->chkAutoHover->isChecked();
    m_plugin->m_onTypeFormatting = ui->chkOnTypeFormatting->isChecked();
    m_plugin->m_incrementalSync = ui->chkIncrementalSync->isChecked();
    m_plugin->m_semanticHighlighting = ui->chkSemanticHighlighting->isChecked();

    m_plugin->m_configPath = ui->edtConfigPath->url();

    // own scope to ensure file is flushed before we signal below in writeConfig!
    {
        QFile configFile(m_plugin->configPath().toLocalFile());
        configFile.open(QIODevice::WriteOnly);
        if (configFile.isOpen()) {
            configFile.write(ui->userConfig->toPlainText().toUtf8());
        }
    }

    m_plugin->writeConfig();
}

void LSPClientConfigPage::reset()
{
    ui->chkSymbolDetails->setChecked(m_plugin->m_symbolDetails);
    ui->chkSymbolTree->setChecked(m_plugin->m_symbolTree);
    ui->chkSymbolExpand->setChecked(m_plugin->m_symbolExpand);
    ui->chkSymbolSort->setChecked(m_plugin->m_symbolSort);

    ui->chkComplDoc->setChecked(m_plugin->m_complDoc);
    ui->chkRefDeclaration->setChecked(m_plugin->m_refDeclaration);

    ui->chkDiagnostics->setChecked(m_plugin->m_diagnostics);
    ui->chkDiagnosticsHighlight->setChecked(m_plugin->m_diagnosticsHighlight);
    ui->chkDiagnosticsMark->setChecked(m_plugin->m_diagnosticsMark);

    ui->chkAutoHover->setChecked(m_plugin->m_autoHover);
    ui->chkOnTypeFormatting->setChecked(m_plugin->m_onTypeFormatting);
    ui->chkIncrementalSync->setChecked(m_plugin->m_incrementalSync);
    ui->chkSemanticHighlighting->setChecked(m_plugin->m_semanticHighlighting);

    ui->edtConfigPath->setUrl(m_plugin->m_configPath);

    readUserConfig(m_plugin->configPath().toLocalFile());
}

void LSPClientConfigPage::defaults()
{
    reset();
}

void LSPClientConfigPage::readUserConfig(const QString &fileName)
{
    QFile configFile(fileName);
    configFile.open(QIODevice::ReadOnly);
    if (configFile.isOpen()) {
        ui->userConfig->setPlainText(QString::fromUtf8(configFile.readAll()));
    } else {
        ui->userConfig->clear();
    }
}

void LSPClientConfigPage::configUrlChanged()
{
    // re-read config
    readUserConfig(ui->edtConfigPath->url().isEmpty() ? m_plugin->m_defaultConfigPath.toLocalFile() : ui->edtConfigPath->url().toLocalFile());

    // remember changed
    changed();
}
