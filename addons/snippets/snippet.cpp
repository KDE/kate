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
#include "ktexteditor_version.h"

#include <KLocalizedString>

#include <QAction>
#include <QApplication>

Snippet::Snippet()
    : QStandardItem(i18n("<empty snippet>"))
{
    setSnippet(QString(), QString(), TextTemplate);
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
    if (string == QLatin1String("script")) {
        return Script;
    }
    // this must be the default for backwards compatibility!
    return TextTemplate;
}

void Snippet::setSnippet(const QString &snippet, const QString &description, SnippetType type)
{
    m_snippet = snippet;
    m_description = description;
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
#if KTEXTEDITOR_VERSION >= QT_VERSION_CHECK(6, 15, 0)
        QVariant res;
        view->evaluateScript(repoScript + u'\n' + snippet(), &res);
        // for convenience, insert result (or error message) at cursor position
        view->document()->insertText(view->cursorPosition(), res.toString());
#else
        // this should not usually be hit, but let's show an error message
        view->document()->insertText(view->cursorPosition(),
                                     i18n("Kate needs to be compiled against KTExtEditor version 6.15.0 or higher, to use this type of snippet."));
#endif
    }
}

QString Snippet::description() const
{
    return m_description;
}

QVariant Snippet::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        return m_description.isEmpty() ? m_snippet : m_description;
    } else if ((role == Qt::ForegroundRole || role == Qt::BackgroundRole) && parent()->checkState() != Qt::Checked) {
        /// TODO: make the selected items also "disabled" so the toggle action is seen directly
        if (role == Qt::ForegroundRole) {
            return qApp->palette().color(QPalette::Disabled, QPalette::Text);
        } else {
            return qApp->palette().color(QPalette::Disabled, QPalette::Base);
        }
    }
    return QStandardItem::data(role);
}
