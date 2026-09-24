/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitforgeconfigwidget.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

GitForgeConfigWidget::GitForgeConfigWidget(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_detectButton(new QPushButton(QIcon::fromTheme(QStringLiteral("tools-wizard")), i18n("Detect Service"), this))
    , m_detectionStatus(new QLabel(this))
{
    auto *layout = new QVBoxLayout(this);

    auto *description = new QLabel(i18n("Map Git remote hosts to their Forgejo, GitHub, or GitLab website. The remote "
                                        "host is taken from the clone URL; the website URL is used to open "
                                        "repository links in a browser."),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({ i18n("Git Remote Host"), i18n("Hosting Service"), i18n("Website URL") });
    m_table->horizontalHeaderItem(0)->setToolTip(i18n("Host from the Git clone URL, optionally followed by its SSH port"));
    m_table->horizontalHeaderItem(1)->setToolTip(i18n("Forgejo, GitHub, or GitLab software running on the server"));
    m_table->horizontalHeaderItem(2)->setToolTip(i18n("HTTP or HTTPS address used to open repository links in a web browser"));
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);

    auto *buttons = new QHBoxLayout;
    auto *addButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Mapping"), this);
    auto *removeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove Selected Mapping"), this);
    removeButton->setEnabled(false);
    m_detectButton->setEnabled(false);
    m_detectButton->setToolTip(i18n("Try to identify Forgejo, GitHub, or GitLab from the "
                                    "selected mapping's website URL"));
    buttons->addWidget(addButton);
    buttons->addWidget(removeButton);
    buttons->addWidget(m_detectButton);
    buttons->addStretch();
    m_detectionStatus->hide();
    buttons->addWidget(m_detectionStatus);
    layout->addLayout(buttons);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        addRow();
        m_table->selectRow(m_table->rowCount() - 1);
        Q_EMIT hostMappingsChanged();
    });
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        const auto rows = m_table->selectionModel()->selectedRows();
        for (auto it = rows.crbegin(); it != rows.crend(); ++it) {
            m_table->removeRow(it->row());
        }
        if (!rows.isEmpty()) {
            Q_EMIT hostMappingsChanged();
        }
    });
    connect(m_detectButton, &QPushButton::clicked, this, &GitForgeConfigWidget::detectService);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this, removeButton]() {
        const bool hasSelection = !m_table->selectionModel()->selectedRows().isEmpty();
        removeButton->setEnabled(hasSelection);
        m_detectButton->setEnabled(hasSelection && !m_detecting);
        if (!m_detecting) {
            m_detectionStatus->hide();
        }
    });
}

std::optional<QList<GitForge::HostMapping>> GitForgeConfigWidget::hostMappings()
{
    QList<GitForge::HostMapping> mappings;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto *providerBox = qobject_cast<QComboBox *>(m_table->cellWidget(row, 1));
        auto *hostEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(row, 0));
        auto *webBaseUrlEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(row, 2));
        const auto provider = providerBox ? GitForge::providerFromName(providerBox->currentText()) : std::nullopt;
        const QString host = hostEdit ? hostEdit->text().trimmed() : QString();
        const QUrl webBaseUrl(webBaseUrlEdit ? webBaseUrlEdit->text().trimmed() : QString(), QUrl::StrictMode);
        if (!provider) {
            KMessageBox::error(this, i18nc("Git hosting service, such as Forgejo, GitHub, or GitLab", "The hosting service in row %1 is invalid.", row + 1));
            return std::nullopt;
        }
        const auto mapping = GitForge::hostMapping(host, *provider, webBaseUrl);
        if (!mapping) {
            KMessageBox::error(this, i18n("The Git remote host or website URL in row %1 is invalid.", row + 1));
            return std::nullopt;
        }
        mappings.push_back(*mapping);
    }
    return mappings;
}

void GitForgeConfigWidget::setHostMappings(const QList<GitForge::HostMapping> &mappings)
{
    const QSignalBlocker blocker(m_table);
    m_table->setRowCount(0);
    for (const GitForge::HostMapping &mapping : mappings) {
        QString host = mapping.host.contains(u':') ? QStringLiteral("[%1]").arg(mapping.host) : mapping.host;
        if (mapping.port != -1) {
            host += QStringLiteral(":%1").arg(mapping.port);
        }
        addRow(host, GitForge::providerName(mapping.provider), mapping.webBaseUrl.toString(QUrl::FullyEncoded));
    }
}

void GitForgeConfigWidget::addRow(const QString &host, const QString &provider, const QString &webBaseUrl)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto *hostEdit = new QLineEdit(host, m_table);
    hostEdit->setPlaceholderText(i18n("git.example.com or git.example.com:2222"));
    hostEdit->setFrame(false);
    connect(hostEdit, &QLineEdit::textChanged, this, &GitForgeConfigWidget::hostMappingsChanged);
    m_table->setCellWidget(row, 0, hostEdit);

    auto *providerBox = new QComboBox(m_table);
    providerBox->addItems({ QStringLiteral("Forgejo"), QStringLiteral("GitHub"), QStringLiteral("GitLab") });
    providerBox->setCurrentText(provider);
    connect(providerBox, &QComboBox::currentTextChanged, this, &GitForgeConfigWidget::hostMappingsChanged);
    connect(providerBox, &QComboBox::currentTextChanged, this, [this]() {
        if (!m_detecting) {
            m_detectionStatus->hide();
        }
    });
    m_table->setCellWidget(row, 1, providerBox);

    auto *webBaseUrlEdit = new QLineEdit(webBaseUrl, m_table);
    webBaseUrlEdit->setPlaceholderText(i18n("https://git.example.com"));
    webBaseUrlEdit->setFrame(false);
    connect(webBaseUrlEdit, &QLineEdit::textChanged, this, &GitForgeConfigWidget::hostMappingsChanged);
    connect(webBaseUrlEdit, &QLineEdit::textChanged, this, [this]() {
        if (!m_detecting) {
            m_detectionStatus->hide();
        }
    });
    m_table->setCellWidget(row, 2, webBaseUrlEdit);
}

void GitForgeConfigWidget::detectService()
{
    const int row = m_table->currentRow();
    auto *providerBox = row >= 0 ? qobject_cast<QComboBox *>(m_table->cellWidget(row, 1)) : nullptr;
    auto *webBaseUrlEdit = row >= 0 ? qobject_cast<QLineEdit *>(m_table->cellWidget(row, 2)) : nullptr;
    if (!providerBox || !webBaseUrlEdit) {
        KMessageBox::error(this, i18n("Select a mapping to detect its hosting service."));
        return;
    }

    m_pendingProbes = GitForge::providerApiUrls(QUrl(webBaseUrlEdit->text().trimmed(), QUrl::StrictMode));
    if (m_pendingProbes.isEmpty()) {
        KMessageBox::error(this,
            i18n("Enter a valid HTTP or HTTPS website URL "
                 "before detecting the hosting service."));
        return;
    }

    m_detectionTarget = providerBox;
    m_detectionUrlEdit = webBaseUrlEdit;
    m_detectionUrlText = webBaseUrlEdit->text();
    m_detectionProviderName = providerBox->currentText();
    m_detecting = true;
    m_detectButton->setEnabled(false);
    m_detectionStatus->setText(i18n("Detecting…"));
    m_detectionStatus->show();
    startNextProbe();
}

void GitForgeConfigWidget::startNextProbe()
{
    if (!m_detectionTarget || m_pendingProbes.isEmpty()) {
        finishDetection(std::nullopt);
        return;
    }

    const auto [provider, url] = m_pendingProbes.takeFirst();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kate Git Forge"));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(5000);
    QNetworkReply *reply = m_networkManager->get(request);
    constexpr qint64 maxApiResponseSize = 64 * 1024;
    reply->setReadBufferSize(maxApiResponseSize);
    QTimer::singleShot(5000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, provider]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const bool responseUsable = reply->error() == QNetworkReply::NoError || statusCode == 401;
        reply->deleteLater();
        if (responseUsable && GitForge::isProviderApiResponse(provider, statusCode, body)) {
            finishDetection(provider);
        } else {
            startNextProbe();
        }
    });
}

void GitForgeConfigWidget::finishDetection(std::optional<GitForge::Provider> provider)
{
    m_pendingProbes.clear();
    const bool targetUnchanged = m_detectionTarget && m_detectionUrlEdit && m_detectionUrlEdit->text() == m_detectionUrlText
        && m_detectionTarget->currentText() == m_detectionProviderName;
    if (provider && targetUnchanged) {
        const QString name = GitForge::providerName(*provider);
        m_detectionTarget->setCurrentText(name);
        m_detectionStatus->setText(i18n("Detected %1.", name));
    } else if (provider) {
        m_detectionStatus->setText(i18n("The mapping changed; the detection result was ignored."));
    } else {
        m_detectionStatus->setText(i18n("Service could not be detected; select it manually."));
    }
    m_detectionTarget.clear();
    m_detectionUrlEdit.clear();
    m_detectionUrlText.clear();
    m_detectionProviderName.clear();
    m_detecting = false;
    m_detectButton->setEnabled(!m_table->selectionModel()->selectedRows().isEmpty());
}
