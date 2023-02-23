/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FormatConfig.h"

#include "FormatPlugin.h"
#include "FormattersEnum.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
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

static void setCurrentIndexForCombo(QComboBox *cmb, int formatter)
{
    for (int i = 0; i < cmb->count(); ++i) {
        if (cmb->itemData(i).value<int>() == formatter) {
            cmb->setCurrentIndex(i);
            return;
        }
    }
    qWarning() << "Invalid formatter for combo" << formatter;
}

class GeneralTab : public QWidget
{
    Q_OBJECT
public:
    GeneralTab(FormatPlugin *plugin, QWidget *parent)
        : QWidget(parent)
        , m_plugin(plugin)
        , m_cbFormatOnSave(new QCheckBox(this))
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins({});

        auto gb = new QGroupBox(i18n("General"), this);
        mainLayout->addWidget(gb);
        { // General
            auto layout = new QVBoxLayout(gb);
            m_cbFormatOnSave->setText(i18n("Format on save"));
            connect(m_cbFormatOnSave, &QCheckBox::stateChanged, this, &GeneralTab::changed);
            layout->addWidget(m_cbFormatOnSave);
        }

        gb = new QGroupBox(i18n("Formatter Preference"), this);
        mainLayout->addWidget(gb);
        {
            auto layout = new QVBoxLayout(gb);
            auto addRow = [layout, this](auto label, auto combo) {
                auto hlayout = new QHBoxLayout;
                connect(combo, &QComboBox::currentIndexChanged, this, &GeneralTab::changed);
                layout->addLayout(hlayout);
                hlayout->setContentsMargins({});
                hlayout->addWidget(label);
                hlayout->addWidget(combo);
                hlayout->addStretch();
            };

            auto lbl = new QLabel(i18n("Formatter for Json"), this);
            m_cmbJson = new QComboBox(this);
            addRow(lbl, m_cmbJson);
            m_cmbJson->addItem(i18n("Clang Format"), (int)Formatters::ClangFormat);
            m_cmbJson->addItem(i18n("Prettier"), (int)Formatters::Prettier);
            m_cmbJson->addItem(i18n("Jq"), (int)Formatters::Jq);
        }

        mainLayout->addStretch();
    }

    bool apply()
    {
        bool changed = false;

        KConfigGroup cg(KSharedConfig::openConfig(), "Formatting");
        if (m_plugin->formatOnSave != m_cbFormatOnSave->isChecked()) {
            m_plugin->formatOnSave = m_cbFormatOnSave->isChecked();
            cg.writeEntry("FormatOnSave", m_plugin->formatOnSave);
            changed = true;
        }

        if ((int)m_plugin->formatterForJson != m_cmbJson->currentData().value<int>()) {
            m_plugin->formatterForJson = (Formatters)m_cmbJson->currentData().value<int>();
            cg.writeEntry("FormatterForJson", (int)m_plugin->formatterForJson);
            changed = true;
        }

        return changed;
    }

    void reset()
    {
        m_cbFormatOnSave->setChecked(m_plugin->formatOnSave);
        setCurrentIndexForCombo(m_cmbJson, (int)m_plugin->formatterForJson);
    }

    Q_SIGNAL void changed();

private:
    FormatPlugin *const m_plugin;
    QCheckBox *const m_cbFormatOnSave;
    QComboBox *m_cmbJson;
};

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

class UserConfigEdit : public QWidget
{
    Q_OBJECT
public:
    UserConfigEdit(FormatPlugin *plugin, QWidget *parent)
        : QWidget(parent)
        , m_plugin(plugin)
    {
        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins({});
        vbox->addWidget(&m_edit);
        vbox->addWidget(&m_errorLabel);
        initTextEdit(&m_edit);

        connect(m_edit.document(), &QTextDocument::contentsChange, this, [this](int, int add, int rem) {
            if (add || rem) {
                m_timer.start();
                Q_EMIT changed();
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
            QJsonDocument doc = QJsonDocument::fromJson(m_edit.toPlainText().toUtf8(), &err);
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

    bool apply()
    {
        QFile f(m_plugin->userConfigPath());
        if (f.open(QFile::WriteOnly)) {
            f.write(m_edit.toPlainText().toUtf8());
            f.flush();
            m_plugin->readFormatterConfig();
            return true;
        }
        return false;
    }

    Q_SIGNAL void changed();

private:
    FormatPlugin *const m_plugin;
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

    m_genTab = new GeneralTab(m_plugin, this);
    connect(m_genTab, &GeneralTab::changed, this, &KTextEditor::ConfigPage::changed);
    m_tabWidget->addTab(m_genTab, i18n("General"));

    m_userConfigEdit = new UserConfigEdit(m_plugin, this);
    connect(m_userConfigEdit, &UserConfigEdit::changed, this, &KTextEditor::ConfigPage::changed);
    m_tabWidget->addTab(m_userConfigEdit, i18n("User Settings"));

    m_defaultConfigEdit = new QPlainTextEdit(this);
    initTextEdit(m_defaultConfigEdit);
    QFile defaultConfigFile(QStringLiteral(":/formatting/FormatterSettings.json"));
    defaultConfigFile.open(QIODevice::ReadOnly);
    Q_ASSERT(defaultConfigFile.isOpen());
    m_defaultConfigEdit->setPlainText(QString::fromUtf8(defaultConfigFile.readAll()));
    m_tabWidget->addTab(m_defaultConfigEdit, i18n("Default Settings"));

    m_tabWidget->setCurrentWidget(m_genTab);

    reset();
}

void FormatConfigPage::apply()
{
    bool changed = m_genTab->apply() || m_userConfigEdit->apply();
    if (changed) {
        m_plugin->configChanged();
    }
}

void FormatConfigPage::reset()
{
    m_genTab->reset();
    m_userConfigEdit->reset();
}

#include "FormatConfig.moc"
