//
// Description: GDB variable parser
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
// SPDX-FileCopyrightText: 2023 Rémi Peuchot <kde.remi@proton.me>
//
// SPDX-License-Identifier: LGPL-2.0-only

#pragma once

#include "dap/entities.h"
#include <QObject>

class GDBVariableParser : public QObject
{
    Q_OBJECT
public:
    GDBVariableParser(QObject *parent = nullptr);

    void insertVariable(const QString &name, const QString &value, const QString &type, bool changed = false);

Q_SIGNALS:
    void variable(int parentId, const dap::Variable &variable);

private:
    int createAndSignalVariable(int parentId, const QStringView name, const QStringView value, const QString &type, bool changed);
    void insertVariable(int parentId, int itemIndex, QStringView &tail, const QString &type, bool changed);
    void insertNamedVariable(int parentId, QStringView name, int itemIndex, QStringView &tail, const QString &type, bool changed);

    int m_variableId = 0;
};
