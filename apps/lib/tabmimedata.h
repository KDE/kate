/*
 *  SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QMimeData>

#include "kateviewspace.h"

#include <optional>

namespace KTextEditor
{
class Document;
}

class TabMimeData : public QMimeData
{
    Q_OBJECT
public:
    struct DroppedData {
        int line = -1;
        int col = -1;
        QUrl url;
    };

    TabMimeData(KateViewSpace *vs, DocOrWidget d);

    static bool hasValidData(const QMimeData *md);

    static std::optional<DroppedData> data(const QMimeData *md);

    KateViewSpace *const sourceVS;
    const DocOrWidget doc;
};
