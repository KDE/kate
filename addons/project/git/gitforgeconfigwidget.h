/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "gitforgeurl.h"

#include <QPointer>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QTableWidget;

class GitForgeConfigWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GitForgeConfigWidget(QWidget *parent);

    std::optional<QList<GitForge::HostMapping>> hostMappings();
    void setHostMappings(const QList<GitForge::HostMapping> &mappings);

Q_SIGNALS:
    void hostMappingsChanged();

private:
    void addRow(const QString &host = {}, const QString &provider = QStringLiteral("GitLab"), const QString &webBaseUrl = {});
    void detectService();
    void startNextProbe();
    void finishDetection(std::optional<GitForge::Provider> provider);

    QTableWidget *m_table;
    QNetworkAccessManager *m_networkManager;
    QPushButton *m_detectButton;
    QLabel *m_detectionStatus;
    QPointer<QComboBox> m_detectionTarget;
    QPointer<QLineEdit> m_detectionUrlEdit;
    QList<QPair<GitForge::Provider, QUrl>> m_pendingProbes;
    QString m_detectionUrlText;
    QString m_detectionProviderName;
    bool m_detecting = false;
};
