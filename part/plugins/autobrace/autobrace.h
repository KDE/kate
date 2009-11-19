/**
  * This file is part of the KDE libraries
  * Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>
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

#ifndef AUTOBRACE_H
#define AUTOBRACE_H

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kpluginfactory.h>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QVariantList>

class AutoBracePlugin
    : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit AutoBracePlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    virtual ~AutoBracePlugin();

    static AutoBracePlugin *self() { return plugin; }

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

    virtual void readConfig (KConfig *) {}
    virtual void writeConfig (KConfig *) {}

  private:
    static AutoBracePlugin *plugin;
    QHash<class KTextEditor::View*, class KTextEditor::Document*> m_documents;
    QHash<class KTextEditor::Document*, class AutoBracePluginDocument*> m_docplugins;
};

class AutoBracePluginDocument
   : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    explicit AutoBracePluginDocument(KTextEditor::Document *document = 0);
    ~AutoBracePluginDocument();

  private Q_SLOTS:
    void slotTextChanged(KTextEditor::Document *document);
    void slotTextInserted(KTextEditor::Document *document, const KTextEditor::Range& range);

  private:
    bool isInsertionCandidate(KTextEditor::Document *document, int openingBraceLine);

  Q_SIGNALS:
    void indent();

  private:
    KTextEditor::Document *m_document;
    int m_insertionLine;
    QString m_indentation;
};

K_PLUGIN_FACTORY_DECLARATION(AutoBracePluginFactory)

#endif // AUTOBRACE_H
