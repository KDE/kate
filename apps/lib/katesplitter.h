/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2021 Michal Humpula <kde@hudrydum.cz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSplitter>

class KateSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit KateSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    explicit KateSplitter(QWidget *parent = nullptr);

protected:
    virtual QSplitterHandle *createHandle() override;
};
