/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Joseph Wenninger <jowenn@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QPlainTextEdit>

class KateProjectPluginView;
class KateProject;

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectInfoViewNotes : public QWidget
{
public:
    /**
     * construct project info view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectInfoViewNotes(QTextDocument *projectNotesDocument);

private:
    /**
     * edit widget bound to notes document of project
     */
    QPlainTextEdit *m_edit;
};
