/*
    SPDX-FileCopyrightText: 2021 Markus Ebner <info@ebner-markus.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "MatchModel.h"
#include "ui_MatchExportDialog.h"

class MatchExportDialog : public QDialog, public Ui::MatchExportDialog
{
    Q_OBJECT

public:
    MatchExportDialog(QWidget *parent, MatchModel *matchModel, QRegularExpression *regExp);

    virtual ~MatchExportDialog();

protected Q_SLOTS:

    void generateMatchExport();

private:
    MatchModel *m_matchModel;
    QRegularExpression *m_regExp;
};
