/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "outputwidget.h"
#include "dataoutputwidget.h"
#include "textoutputwidget.h"
#include <KActionCollection>
#include <KLocalizedString>

KateSQLOutputWidget::KateSQLOutputWidget(QWidget *parent, KActionCollection *actionCollection)
    : QTabWidget(parent)

{
    setDocumentMode(true);
    addTab(m_textOutputWidget = new TextOutputWidget(this),
           QIcon::fromTheme(QStringLiteral("view-list-text")),
           i18nc("@title:window", "SQL Text Output")); // TODO better Icon from QIcon::ThemeIcon::...
    addTab(m_dataOutputWidget = new DataOutputWidget(this, actionCollection),
           QIcon::fromTheme(QStringLiteral("view-form-table")),
           i18nc("@title:window", "SQL Data Output")); // TODO better Icon from QIcon::ThemeIcon::...
}

KateSQLOutputWidget::~KateSQLOutputWidget() = default;
