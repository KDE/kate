/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2021 Michal Humpula <kde@hudrydum.cz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __KATE_SPLITTER_H__
#define __KATE_SPLITTER_H__

#include <QSplitter>

class KateSplitter : public QSplitter
{
    Q_OBJECT

public:
    KateSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    KateSplitter(QWidget *parent = nullptr);

protected:
    virtual QSplitterHandle *createHandle() override;
};

#endif
