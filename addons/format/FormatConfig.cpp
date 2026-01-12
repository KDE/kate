/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FormatConfig.h"

#include "FormatPlugin.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KTextEditor/Editor>

static void initTextEdit(QPlainTextEdit *edit)
{
    edit->setFont(KTextEditor::Editor::instance()->font());
    auto hl = new KSyntaxHighlighting::SyntaxHighlighter(edit->document());
    hl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(QStringLiteral("FormatterSettings.json")));
    // we want to have the proper theme for the current palette
    const auto theme = KTextEditor::Editor::instance()->theme();
    auto pal = edit->palette();
    pal.setColor(QPalette::Base, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)));
    pal.setColor(QPalette::Highlight, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection)));
    edit->setPalette(pal);
    hl->setTheme(theme);
}

class UserConfigEdit final : public QWidget
{
public:
    UserConfigEdit(FormatPlugin *plugin, FormatConfigPage *parent)
        : QWidget(parent)
        , m_plugin(plugin)
        , m_parent(parent)
    {
        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins({});
        vbox->addWidget(&m_edit);
        vbox->addWidget(&m_errorLabel);
        initTextEdit(&m_edit);

        connect(m_edit.document(), &QTextDocument::contentsChange, this, [this](int, int add, int rem) {
            if (add || rem) {
                m_timer.start();
                Q_EMIT m_parent->changed();
            }
        });

        m_timer.setInterval(1500);
        m_timer.setSingleShot(true);
        m_timer.callOnTimeout(this, [this] {
            if (m_edit.document()->isEmpty()) {
                m_errorLabel.setVisible(false);
                m_errorLabel.clear();
                return;
            }
            QJsonParseError err;
            QJsonDocument::fromJson(m_edit.toPlainText().toUtf8(), &err);
            if (err.error != QJsonParseError::NoError) {
                m_errorLabel.setText(err.errorString());
                m_errorLabel.setVisible(true);
            }
        });
    }

    void reset()
    {
        QFile f(m_plugin->userConfigPath());
        if (f.open(QFile::ReadOnly)) {
            m_edit.setPlainText(QString::fromUtf8(f.readAll()));
            m_timer.start();
        } else {
            m_edit.clear();
        }
    }

    void apply()
    {
        QFile f(m_plugin->userConfigPath());
        if (f.open(QFile::WriteOnly)) {
            f.write(m_edit.toPlainText().toUtf8());
            f.flush();
            m_plugin->readConfig();
        }
    }

private:
    FormatPlugin *const m_plugin;
    FormatConfigPage *const m_parent;
    QPlainTextEdit m_edit;
    QLabel m_errorLabel;
    QTimer m_timer;
};

FormatConfigPage::FormatConfigPage(class FormatPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
    , m_tabWidget(new QTabWidget(this))
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    m_tabWidget->setContentsMargins({});
    layout->addWidget(m_tabWidget);

    m_userConfigEdit = new UserConfigEdit(m_plugin, this);
    m_tabWidget->addTab(m_userConfigEdit, i18n("User Settings"));

    m_defaultConfigEdit = new QPlainTextEdit(this);
    initTextEdit(m_defaultConfigEdit);
    QFile defaultConfigFile(QStringLiteral(":/formatting/FormatterSettings.json"));
    if (!defaultConfigFile.open(QIODevice::ReadOnly)) {
        Q_UNREACHABLE();
    }
    Q_ASSERT(defaultConfigFile.isOpen());
    m_defaultConfigEdit->setPlainText(QString::fromUtf8(defaultConfigFile.readAll()));
    m_tabWidget->addTab(m_defaultConfigEdit, i18n("Default Settings"));

    m_tabWidget->setCurrentWidget(m_userConfigEdit);

    reset();
}

void FormatConfigPage::apply()
{
    m_userConfigEdit->apply();
    m_plugin->configChanged();
}

void FormatConfigPage::reset()
{
    m_userConfigEdit->reset();
}
