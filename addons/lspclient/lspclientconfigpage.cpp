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

#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KUrlRequester>

LSPClientConfigPage::LSPClientConfigPage(QWidget *parent, LSPClientPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *outlineBox = new QGroupBox(i18n("Symbol Outline Options"), this);
    QVBoxLayout *top = new QVBoxLayout(outlineBox);
    m_symbolDetails = new QCheckBox(i18n("Display symbol details"));
    m_symbolTree = new QCheckBox(i18n("Tree mode outline"));
    m_symbolExpand = new QCheckBox(i18n("Automatically expand nodes in tree mode"));
    m_symbolSort = new QCheckBox(i18n("Sort symbols alphabetically"));
    top->addWidget(m_symbolDetails);
    top->addWidget(m_symbolTree);
    top->addWidget(m_symbolExpand);
    top->addWidget(m_symbolSort);
    layout->addWidget(outlineBox);

    outlineBox = new QGroupBox(i18n("General Options"), this);
    top = new QVBoxLayout(outlineBox);
    m_complDoc = new QCheckBox(i18n("Show selected completion documentation"));
    m_refDeclaration = new QCheckBox(i18n("Include declaration in references"));
    m_autoHover = new QCheckBox(i18n("Show hover information"));
    m_onTypeFormatting = new QCheckBox(i18n("Format on typing"));
    m_incrementalSync = new QCheckBox(i18n("Incremental document synchronization"));
    QHBoxLayout *diagLayout = new QHBoxLayout();
    m_diagnostics = new QCheckBox(i18n("Show diagnostics notifications"));
    m_diagnosticsHighlight = new QCheckBox(i18n("Add highlights"));
    m_diagnosticsMark = new QCheckBox(i18n("Add markers"));
    top->addWidget(m_complDoc);
    top->addWidget(m_refDeclaration);
    top->addWidget(m_autoHover);
    top->addWidget(m_onTypeFormatting);
    top->addWidget(m_incrementalSync);
    diagLayout->addWidget(m_diagnostics);
    diagLayout->addStretch(1);
    diagLayout->addWidget(m_diagnosticsHighlight);
    diagLayout->addStretch(1);
    diagLayout->addWidget(m_diagnosticsMark);
    top->addLayout(diagLayout);
    layout->addWidget(outlineBox);

    outlineBox = new QGroupBox(i18n("Server Configuration"), this);
    top = new QVBoxLayout(outlineBox);
    m_configPath = new KUrlRequester(this);
    top->addWidget(m_configPath);
    layout->addWidget(outlineBox);

    layout->addStretch(1);

    reset();

    for (const auto &cb : {m_symbolDetails, m_symbolExpand, m_symbolSort, m_symbolTree, m_complDoc, m_refDeclaration, m_diagnostics, m_diagnosticsMark, m_onTypeFormatting, m_incrementalSync, m_autoHover})
        connect(cb, &QCheckBox::toggled, this, &LSPClientConfigPage::changed);
    connect(m_configPath, &KUrlRequester::textChanged, this, &LSPClientConfigPage::changed);
    connect(m_configPath, &KUrlRequester::urlSelected, this, &LSPClientConfigPage::changed);

    // custom control logic
    auto h = [this]() {
        bool enabled = m_diagnostics->isChecked();
        m_diagnosticsHighlight->setEnabled(enabled);
        m_diagnosticsMark->setEnabled(enabled);
    };
    connect(this, &LSPClientConfigPage::changed, this, h);
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
    m_plugin->m_symbolDetails = m_symbolDetails->isChecked();
    m_plugin->m_symbolTree = m_symbolTree->isChecked();
    m_plugin->m_symbolExpand = m_symbolExpand->isChecked();
    m_plugin->m_symbolSort = m_symbolSort->isChecked();

    m_plugin->m_complDoc = m_complDoc->isChecked();
    m_plugin->m_refDeclaration = m_refDeclaration->isChecked();

    m_plugin->m_diagnostics = m_diagnostics->isChecked();
    m_plugin->m_diagnosticsHighlight = m_diagnosticsHighlight->isChecked();
    m_plugin->m_diagnosticsMark = m_diagnosticsMark->isChecked();

    m_plugin->m_autoHover = m_autoHover->isChecked();
    m_plugin->m_onTypeFormatting = m_onTypeFormatting->isChecked();
    m_plugin->m_incrementalSync = m_incrementalSync->isChecked();

    m_plugin->m_configPath = m_configPath->url();

    m_plugin->writeConfig();
}

void LSPClientConfigPage::reset()
{
    m_symbolDetails->setChecked(m_plugin->m_symbolDetails);
    m_symbolTree->setChecked(m_plugin->m_symbolTree);
    m_symbolExpand->setChecked(m_plugin->m_symbolExpand);
    m_symbolSort->setChecked(m_plugin->m_symbolSort);

    m_complDoc->setChecked(m_plugin->m_complDoc);
    m_refDeclaration->setChecked(m_plugin->m_refDeclaration);

    m_diagnostics->setChecked(m_plugin->m_diagnostics);
    m_diagnosticsHighlight->setChecked(m_plugin->m_diagnosticsHighlight);
    m_diagnosticsMark->setChecked(m_plugin->m_diagnosticsMark);

    m_autoHover->setChecked(m_plugin->m_autoHover);
    m_onTypeFormatting->setChecked(m_plugin->m_onTypeFormatting);
    m_incrementalSync->setChecked(m_plugin->m_incrementalSync);

    m_configPath->setUrl(m_plugin->m_configPath);
}

void LSPClientConfigPage::defaults()
{
    reset();
}
