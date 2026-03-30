/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Leia <leia@tutamail.com>

    SPDX-License-Identifier: MIT
*/

#include "jsonsettings.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KUrlRequester>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

JSONSettings::JSONSettings(QObject *parent, QTabWidget *tabWidget, const QString &defaultConfigPath, const QUrl &defaultUserConfigPath)
    : QObject(parent)
    , m_defaultUserConfigPath(defaultUserConfigPath)
{
    // set up UI
    QWidget *userSettingsTab = new QWidget(tabWidget);
    tabWidget->addTab(userSettingsTab, i18n("User Settings"));

    auto *userSettingsLayout = new QVBoxLayout(userSettingsTab);
    userSettingsLayout->setContentsMargins(0, 0, 0, 0);

    auto *headerLayout = new QGridLayout();
    headerLayout->setContentsMargins(6, 4, 6, 4);
    userSettingsLayout->addLayout(headerLayout);

    auto *label = new QLabel(i18n("Settings file:"), userSettingsTab);
    headerLayout->addWidget(label, 0, 0);

    m_configPathEditor = new KUrlRequester(userSettingsTab);
    headerLayout->addWidget(m_configPathEditor, 0, 1);
    // setup default config path as placeholder to show user where it is
    m_configPathEditor->setPlaceholderText(defaultUserConfigPath.toLocalFile());

    connect(m_configPathEditor, &KUrlRequester::textChanged, this, [this]() {
        readUserConfig();
        Q_EMIT JSONSettings::configUrlChanged();
    });
    connect(m_configPathEditor, &KUrlRequester::urlSelected, this, [this]() {
        readUserConfig();
        Q_EMIT JSONSettings::configUrlChanged();
    });

    m_userConfigError = new QLabel(userSettingsTab);
    headerLayout->addWidget(m_userConfigError, 1, 0, 1, 0);

    auto *editor = KTextEditor::Editor::instance();

    m_userConfigDoc = editor->createDocument(tabWidget);

    m_userConfigView = m_userConfigDoc->createView(tabWidget);
    m_userConfigView->setContextMenu(m_userConfigView->defaultContextMenu());
    userSettingsLayout->addWidget(m_userConfigView);

    connect(m_userConfigDoc, &KTextEditor::Document::textChanged, this, [this]() {
        updateConfigTextErrorState();
        Q_EMIT configChanged();
    });

    connect(m_userConfigDoc, &KTextEditor::Document::documentSavedOrUploaded, this, &JSONSettings::configSaved);

    auto *defaultSettingsTab = new QWidget(tabWidget);
    tabWidget->addTab(defaultSettingsTab, i18n("Default Settings"));
    auto *defaultSettingsLayout = new QVBoxLayout(defaultSettingsTab);
    defaultSettingsLayout->setContentsMargins(0, 0, 0, 0);

    auto *defaultConfigDoc = editor->createDocument(tabWidget);
    defaultConfigDoc->setMode(QStringLiteral("JSON"));
    // setup default json settings
    QFile defaultConfigFile(defaultConfigPath);
    if (!defaultConfigFile.open(QIODevice::ReadOnly)) {
        Q_UNREACHABLE();
    }
    Q_ASSERT(defaultConfigFile.isOpen());
    defaultConfigDoc->setText(QString::fromUtf8(defaultConfigFile.readAll()));
    defaultConfigDoc->setReadWrite(false);

    auto *defaultConfigView = defaultConfigDoc->createView(tabWidget);
    defaultConfigView->setContextMenu(defaultConfigView->defaultContextMenu());
    defaultConfigView->setStatusBarEnabled(false);
    defaultSettingsLayout->addWidget(defaultConfigView);

    readUserConfig();
}

QUrl JSONSettings::userConfigPath()
{
    return m_configPathEditor->url();
}

void JSONSettings::setConfigUrl(QUrl &url)
{
    m_configPathEditor->setUrl(url);
}

void JSONSettings::saveUserConfig()
{
    m_userConfigDoc->save();
}

void JSONSettings::readUserConfig()
{
    m_userConfigDoc->openUrl(m_configPathEditor->url().isEmpty() ? m_defaultUserConfigPath : m_configPathEditor->url());
    m_userConfigDoc->setMode(QStringLiteral("JSON"));

    updateConfigTextErrorState();
}

void JSONSettings::updateConfigTextErrorState()
{
    const auto userConfigJsonTxt = m_userConfigDoc->text().toUtf8();
    if (userConfigJsonTxt.isEmpty()) {
        m_userConfigError->setText(i18n("No JSON data to validate."));
        return;
    }

    // check json validity
    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(userConfigJsonTxt, &error);
    if (error.error == QJsonParseError::NoError) {
        if (json.isObject()) {
            m_userConfigError->setText(i18n("JSON data is valid."));
        } else {
            m_userConfigError->setText(i18n("JSON data is invalid: no JSON object"));
        }
    } else {
        m_userConfigError->setText(i18n("JSON data is invalid: %1", error.errorString()));
    }
}

#include "moc_jsonsettings.cpp"
