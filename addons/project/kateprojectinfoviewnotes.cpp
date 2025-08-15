/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Joseph Wenninger <jowenn@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectinfoviewnotes.h"
#include "kateprojectpluginview.h"

#include <QVBoxLayout>

KateProjectInfoViewNotes::KateProjectInfoViewNotes(QTextDocument *projectNotesDocument)
    : m_edit(new QPlainTextEdit())
{
    /*
     * layout widget
     */
    auto *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_edit);
    setLayout(layout);
    m_edit->setDocument(projectNotesDocument);
    setFocusProxy(m_edit);
}
