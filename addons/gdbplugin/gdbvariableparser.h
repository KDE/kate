//
// Description: GDB variable parser (refactored from localsview)
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef GDB_VARIABLE_PARSER_H_
#define GDB_VARIABLE_PARSER_H_

#include "dap/entities.h"
#include <QObject>

class GDBVariableParser : public QObject
{
    Q_OBJECT
public:
    GDBVariableParser(QObject *parent = nullptr);

public Q_SLOTS:
    void addLocal(const QString &vString);

Q_SIGNALS:
    // flat variable
    void variable(int parentId, const dap::Variable &variable);
    void scopeOpened();
    void scopeClosed();

private:
    void addStruct(int parentId, const QString &vString);
    void addArray(int parentId, const QString &vString);

    void openScope();
    void closeScope();

    int m_varId = 1;
    bool m_allAdded = true;
    QString m_local;
};

#endif
