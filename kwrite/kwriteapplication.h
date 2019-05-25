/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KWRITE_APPLICATION_H
#define KWRITE_APPLICATION_H

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>

class KWrite;

class KWriteApplication : public QObject
{
    Q_OBJECT

public:
    KWriteApplication();
    ~KWriteApplication();

    void addDocument(KTextEditor::Document *doc) { m_documents.append(doc); }
    void removeDocument(KTextEditor::Document *doc) { m_documents.removeAll(doc); }
    void removeWindow(KWrite *kwrite) { m_kwrites.removeAll(kwrite); }

    bool noWindows() { return m_kwrites.isEmpty(); }

    KWrite *newWindow(KTextEditor::Document *doc = nullptr);

    void restore();
    void saveProperties(KConfig *config);

public Q_SLOTS:
    QList<KTextEditor::Document *> documents() { return m_documents; }
    bool quit();
    KTextEditor::MainWindow *activeMainWindow();
    QList<KTextEditor::MainWindow *> mainWindows();
    bool closeDocument(KTextEditor::Document *document);

private:
    KTextEditor::Application *m_application;
    QList<KTextEditor::Document *> m_documents;
    QList<KWrite *> m_kwrites;
};

#endif
