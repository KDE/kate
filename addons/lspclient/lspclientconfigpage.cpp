/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "lspclientconfigpage.h"
#include "lspclientplugin.h"
#include "ui_lspconfigwidget.h"

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

LSPClientConfigPage::LSPClientConfigPage(QWidget *parent, LSPClientPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::LspConfigWidget();
    ui->setupUi(this);
    ui->tabWidget->setDocumentMode(true);

    // fix-up our two text edits to be proper JSON file editors
    updateHighlighters();

    // ensure we update the highlighters if the repository is updated or theme is changed
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::repositoryReloaded, this, &LSPClientConfigPage::updateHighlighters);
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &LSPClientConfigPage::updateHighlighters);

    // setup default json settings
    QFile defaultConfigFile(QStringLiteral(":/lspclient/settings.json"));
    defaultConfigFile.open(QIODevice::ReadOnly);
    Q_ASSERT(defaultConfigFile.isOpen());
    ui->defaultConfig->setPlainText(QString::fromUtf8(defaultConfigFile.readAll()));

    // setup default config path as placeholder to show user where it is
    ui->edtConfigPath->setPlaceholderText(m_plugin->m_defaultConfigPath.toLocalFile());

    reset();

    for (const auto &cb : {
             ui->chkSymbolDetails,
             ui->chkSymbolExpand,
             ui->chkSymbolSort,
             ui->chkSymbolTree,
             ui->chkComplDoc,
             ui->chkRefDeclaration,
             ui->chkComplParens,
             ui->chkMessages,
             ui->chkDiagnostics,
             ui->chkOnTypeFormatting,
             ui->chkIncrementalSync,
             ui->chkHighlightGoto,
             ui->chkSemanticHighlighting,
             ui->chkAutoHover,
             ui->chkSignatureHelp,
             ui->chkAutoImport,
             ui->chkFmtOnSave,
             ui->chkInlayHint,
             ui->chkShowCompl,
             ui->chkHighlightSymbol,
         }) {
        connect(cb, &QCheckBox::toggled, this, &LSPClientConfigPage::changed);
    }

    connect(ui->edtConfigPath, &KUrlRequester::textChanged, this, &LSPClientConfigPage::configUrlChanged);
    connect(ui->edtConfigPath, &KUrlRequester::urlSelected, this, &LSPClientConfigPage::configUrlChanged);

    connect(ui->allowedAndBlockedServers, &QListWidget::itemChanged, this, &LSPClientConfigPage::changed);

    // own context menu to delete entries
    ui->allowedAndBlockedServers->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->allowedAndBlockedServers, &QWidget::customContextMenuRequested, this, &LSPClientConfigPage::showContextMenuAllowedBlocked);

    auto cfgh = [this](int position, int added, int removed) {
        Q_UNUSED(position);
        // discard format change
        // (e.g. due to syntax highlighting)
        if (added || removed) {
            configTextChanged();
        }
    };
    connect(ui->userConfig->document(), &QTextDocument::contentsChange, this, cfgh);
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
    return QIcon::fromTheme(QLatin1String("format-text-code"));
}

void LSPClientConfigPage::apply()
{
    m_plugin->m_symbolDetails = ui->chkSymbolDetails->isChecked();
    m_plugin->m_symbolTree = ui->chkSymbolTree->isChecked();
    m_plugin->m_symbolExpand = ui->chkSymbolExpand->isChecked();
    m_plugin->m_symbolSort = ui->chkSymbolSort->isChecked();

    m_plugin->m_showCompl = ui->chkShowCompl->isChecked();
    m_plugin->m_complDoc = ui->chkComplDoc->isChecked();
    m_plugin->m_refDeclaration = ui->chkRefDeclaration->isChecked();
    m_plugin->m_complParens = ui->chkComplParens->isChecked();

    m_plugin->m_autoHover = ui->chkAutoHover->isChecked();
    m_plugin->m_onTypeFormatting = ui->chkOnTypeFormatting->isChecked();
    m_plugin->m_incrementalSync = ui->chkIncrementalSync->isChecked();
    m_plugin->m_highlightGoto = ui->chkHighlightGoto->isChecked();
    m_plugin->m_semanticHighlighting = ui->chkSemanticHighlighting->isChecked();
    m_plugin->m_signatureHelp = ui->chkSignatureHelp->isChecked();
    m_plugin->m_autoImport = ui->chkAutoImport->isChecked();
    m_plugin->m_fmtOnSave = ui->chkFmtOnSave->isChecked();
    m_plugin->m_inlayHints = ui->chkInlayHint->isChecked();
    m_plugin->m_highLightSymbol = ui->chkHighlightSymbol->isChecked();

    m_plugin->m_diagnostics = ui->chkDiagnostics->isChecked();
    m_plugin->m_messages = ui->chkMessages->isChecked();

    m_plugin->m_configPath = ui->edtConfigPath->url();

    m_plugin->m_serverCommandLineToAllowedState.clear();
    for (int i = 0; i < ui->allowedAndBlockedServers->count(); ++i) {
        const auto item = ui->allowedAndBlockedServers->item(i);
        m_plugin->m_serverCommandLineToAllowedState.emplace(item->text(), item->checkState() == Qt::Checked);
    }

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

void LSPClientConfigPage::resetUiTo(const LSPClientPluginOptions &options)
{
    ui->chkSymbolDetails->setChecked(options.m_symbolDetails);
    ui->chkSymbolTree->setChecked(options.m_symbolTree);
    ui->chkSymbolExpand->setChecked(options.m_symbolExpand);
    ui->chkSymbolSort->setChecked(options.m_symbolSort);

    ui->chkShowCompl->setChecked(options.m_showCompl);
    ui->chkComplDoc->setChecked(options.m_complDoc);
    ui->chkRefDeclaration->setChecked(options.m_refDeclaration);
    ui->chkComplParens->setChecked(options.m_complParens);

    ui->chkAutoHover->setChecked(options.m_autoHover);
    ui->chkOnTypeFormatting->setChecked(options.m_onTypeFormatting);
    ui->chkIncrementalSync->setChecked(options.m_incrementalSync);
    ui->chkHighlightGoto->setChecked(options.m_highlightGoto);
    ui->chkSemanticHighlighting->setChecked(options.m_semanticHighlighting);
    ui->chkSignatureHelp->setChecked(options.m_signatureHelp);
    ui->chkAutoImport->setChecked(options.m_autoImport);
    ui->chkFmtOnSave->setChecked(options.m_fmtOnSave);
    ui->chkInlayHint->setChecked(options.m_inlayHints);
    ui->chkHighlightSymbol->setChecked(options.m_highLightSymbol);

    ui->chkDiagnostics->setChecked(options.m_diagnostics);
    ui->chkMessages->setChecked(options.m_messages);
}

void LSPClientConfigPage::reset()
{
    resetUiTo(*m_plugin);

    ui->edtConfigPath->setUrl(m_plugin->m_configPath);

    readUserConfig(m_plugin->configPath().toLocalFile());

    // fill in the allowed and blocked servers
    ui->allowedAndBlockedServers->clear();
    for (const auto &it : m_plugin->m_serverCommandLineToAllowedState) {
        auto item = new QListWidgetItem(it.first, ui->allowedAndBlockedServers);
        item->setCheckState(it.second ? Qt::Checked : Qt::Unchecked);
    }
}

void LSPClientConfigPage::defaults()
{
    resetUiTo(LSPClientPluginOptions{});

    // The user can easily manually revert the list of Allowed & Blocked Servers and the User Server Settings
    // to their empty defaults. So do not automatically clear the possibly valuable user data on the other tabs.
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

    updateConfigTextErrorState();
}

void LSPClientConfigPage::updateConfigTextErrorState()
{
    const auto data = ui->userConfig->toPlainText().toUtf8();
    if (data.isEmpty()) {
        ui->userConfigError->setText(i18n("No JSON data to validate."));
        return;
    }

    // check json validity
    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(data, &error);
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

void LSPClientConfigPage::configTextChanged()
{
    // check for errors
    updateConfigTextErrorState();

    // remember changed
    changed();
}

void LSPClientConfigPage::configUrlChanged()
{
    // re-read config
    readUserConfig(ui->edtConfigPath->url().isEmpty() ? m_plugin->m_defaultConfigPath.toLocalFile() : ui->edtConfigPath->url().toLocalFile());

    // remember changed
    changed();
}

void LSPClientConfigPage::updateHighlighters()
{
    for (auto textEdit : {ui->userConfig, ui->defaultConfig}) {
        // setup JSON highlighter for the default json stuff
        auto highlighter = new KSyntaxHighlighting::SyntaxHighlighter(textEdit->document());
        highlighter->setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(QStringLiteral("settings.json")));

        // we want mono-spaced font
        textEdit->setFont(KTextEditor::Editor::instance()->font());

        // we want to have the proper theme for the current palette
        const auto theme = KTextEditor::Editor::instance()->theme();
        auto pal = qApp->palette();
        pal.setColor(QPalette::Base, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)));
        pal.setColor(QPalette::Highlight, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection)));
        textEdit->setPalette(pal);
        highlighter->setTheme(theme);
        highlighter->rehighlight();
    }
}

void LSPClientConfigPage::showContextMenuAllowedBlocked(const QPoint &pos)
{
    // allow deletion of stuff
    QMenu myMenu(this);

    auto currentDelete = myMenu.addAction(i18n("Delete selected entries"), this, [this]() {
        qDeleteAll(ui->allowedAndBlockedServers->selectedItems());
        Q_EMIT changed();
    });
    currentDelete->setEnabled(!ui->allowedAndBlockedServers->selectedItems().isEmpty());

    auto allDelete = myMenu.addAction(i18n("Delete all entries"), this, [this]() {
        ui->allowedAndBlockedServers->clear();
        Q_EMIT changed();
    });
    allDelete->setEnabled(ui->allowedAndBlockedServers->count() > 0);

    myMenu.exec(ui->allowedAndBlockedServers->mapToGlobal(pos));
}
