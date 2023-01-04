/*
 *
 *  SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QVariant>

#include <KTextEditor/Plugin>

class KatePythonPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    KatePythonPlugin(QObject * = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KatePythonPlugin() = default;
    QObject *createView(KTextEditor::MainWindow *) override;
};
