/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2017 Héctor Mesa Jiménez <hector@lcc.uma.es>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectcodeanalysisselector.h"

#include "kateprojectcodeanalysistoolcppcheck.h"
#include "kateprojectcodeanalysistoolflake8.h"

QStandardItemModel *KateProjectCodeAnalysisSelector::model(QObject *parent)
{
    auto model = new QStandardItemModel(parent);

    /*
     * available linters
     */
    const QList<KateProjectCodeAnalysisTool*> tools = {
        // cppcheck, for C++
        new KateProjectCodeAnalysisToolCppcheck(model),
        // flake8, for Python
        new KateProjectCodeAnalysisToolFlake8(model)
    };

    QList<QStandardItem*> column;

    for (auto tool : tools) {
        auto item = new QStandardItem(tool->name());
        item->setData(QVariant::fromValue(tool), Qt::UserRole + 1);

        column << item;
    }

    model->appendColumn(column);

    return model;
}

