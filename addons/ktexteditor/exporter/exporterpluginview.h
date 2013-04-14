/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef EXPORTERPLUGINVIEW_H
#define EXPORTERPLUGINVIEW_H

#include <QtCore/QObject>
#include <kxmlguiclient.h>

#include <ktexteditor/range.h>

namespace KTextEditor {
class View;
}

class ExporterPluginView : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    ExporterPluginView(KTextEditor::View* view = 0);
    ~ExporterPluginView();

  private:
    ///TODO: maybe make this scriptable for additional exporters?
    void exportData(const bool useSelction, QTextStream& output);

  private slots:
    void exportToClipboard();
    void exportToFile();

    void updateSelectionAction(KTextEditor::View *view);

  private:
    KTextEditor::View* m_view;
    QAction* m_copyAction;
    QAction* m_fileExportAction;
};

#endif // EXPORTERPLUGINVIEW_H

// kate: space-indent on; indent-width 2; replace-tabs on;
