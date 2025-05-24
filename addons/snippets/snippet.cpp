/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  SPDX-FileCopyrightText: 2007 Robert Gruber <rgruber@users.sourceforge.net>
 *  SPDX-FileCopyrightText: 2010 Milian Wolff <mail@milianw.de>
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "snippet.h"
#include "katesnippetglobal.h"
#include "ktexteditor/document.h"
#include "ktexteditor/mainwindow.h"

#include <KLocalizedString>

#include <QAction>
#include <QApplication>

Snippet::Snippet()
    : QStandardItem(i18n("<empty snippet>"))
{
    setSnippet(QString(), TextTemplate);
}

Snippet::~Snippet()
{
    delete m_action;
}

QString Snippet::snippet() const
{
    return m_snippet;
}

Snippet::SnippetType Snippet::snippetType() const
{
    return m_type;
}

// static
QString Snippet::typeToString(const SnippetType type)
{
    if (type == Script) {
        return QStringLiteral("script");
    }
    return QStringLiteral("template");
}

// static
Snippet::SnippetType Snippet::stringToType(const QString &string)
{
    if (string == QStringLiteral("script")) {
        return Script;
    }
    // this must be the default for backwards compatibility!
    return TextTemplate;
}

void Snippet::setSnippet(const QString &snippet, SnippetType type)
{
    m_snippet = snippet;
    m_type = type;
    if (type == TextTemplate) {
        setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));
    } else {
        setIcon(QIcon::fromTheme(QStringLiteral("code-function")));
    }
}

void Snippet::registerActionForView(QWidget *view)
{
    if (view->actions().contains(m_action)) {
        return;
    }
    view->addAction(m_action);
}

QAction *Snippet::action()
{
    /// TODO: this is quite ugly, or is it? if someone knows how to do it better, please refactor
    if (!m_action) {
        static int actionCount = 0;
        actionCount += 1;
        m_action = new QAction(QStringLiteral("insertSnippet%1").arg(actionCount), KateSnippetGlobal::self());
        m_action->setData(QVariant::fromValue<Snippet *>(this));
        KateSnippetGlobal::self()->connect(m_action, &QAction::triggered, KateSnippetGlobal::self(), &KateSnippetGlobal::insertSnippetFromActionData);
    }
    m_action->setText(i18n("insert snippet %1", text()));
    return m_action;
}

void Snippet::apply(KTextEditor::View *view, const QString &repoScript) const
{
    if (snippetType() == TextTemplate) {
        view->insertTemplate(view->cursorPosition(), snippet(), repoScript);
    } else {
        QVariant res;
        auto ok = view->evaluateScript(repoScript + u'\n' + snippet(), &res);
        if (!ok) {
            // show error message, by simply inserting it
            view->document()->insertText(view->cursorPosition(), res.toString());
        }
    }
}

QVariant Snippet::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        return m_snippet;
    } else if ((role == Qt::ForegroundRole || role == Qt::BackgroundRole) && parent()->checkState() != Qt::Checked) {
        /// TODO: make the selected items also "disalbed" so the toggle action is seen directly
        if (role == Qt::ForegroundRole) {
            return qApp->palette().color(QPalette::Disabled, QPalette::Text);
        } else {
            return qApp->palette().color(QPalette::Disabled, QPalette::Base);
        }
    }
    return QStandardItem::data(role);
}
