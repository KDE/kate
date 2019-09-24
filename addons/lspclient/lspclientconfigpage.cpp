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

LSPClientConfigPage::LSPClientConfigPage(QWidget *parent, LSPClientPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::LspConfigWidget();
    ui->setupUi(this);

    reset();

    for (const auto &cb : {ui->chkSymbolDetails, ui->chkSymbolExpand, ui->chkSymbolSort, ui->chkSymbolTree, ui->chkComplDoc, ui->chkRefDeclaration, ui->chkDiagnostics, ui->chkDiagnosticsMark, ui->chkOnTypeFormatting, ui->chkIncrementalSync, ui->chkAutoHover})
        connect(cb, &QCheckBox::toggled, this, &LSPClientConfigPage::changed);
    connect(ui->edtConfigPath, &KUrlRequester::textChanged, this, &LSPClientConfigPage::changed);
    connect(ui->edtConfigPath, &KUrlRequester::urlSelected, this, &LSPClientConfigPage::changed);

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

    m_plugin->m_configPath = ui->edtConfigPath->url();

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

    ui->edtConfigPath->setUrl(m_plugin->m_configPath);
}

void LSPClientConfigPage::defaults()
{
    reset();
}
