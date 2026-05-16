/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "schemabrowserwidget.h"
#include "schemawidget.h"

#include <KLocalizedString>

#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QVBoxLayout>

SchemaBrowserWidget::SchemaBrowserWidget(QWidget *parent, SQLManager *manager)
    : QWidget(parent)
    , m_schemaWidget(new SchemaWidget(this, manager))
    , m_searchInput(new QLineEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchInput->setPlaceholderText(i18n("Filter tables and views..."));
    m_searchInput->hide();
    layout->addWidget(m_searchInput);

    layout->addWidget(m_schemaWidget);
    setLayout(layout);

    connect(m_searchInput, &QLineEdit::textEdited, this, &SchemaBrowserWidget::searchFieldCommit);

    // Install event filter to intercept Ctrl+F, Enter, Esc
    m_schemaWidget->viewport()->installEventFilter(this);
    m_schemaWidget->installEventFilter(this);
    m_searchInput->installEventFilter(this);
    installEventFilter(this);
}

SchemaBrowserWidget::~SchemaBrowserWidget() = default;

SchemaWidget *SchemaBrowserWidget::schemaWidget() const
{
    return m_schemaWidget;
}

bool SchemaBrowserWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto *ke = static_cast<QKeyEvent *>(event);
        const QKeySequence eventShortcut(ke->keyCombination());
        if (eventShortcut == QKeySequence(Qt::CTRL | Qt::Key_F)) {
            // Override Ctrl+F when schema browser area has focus
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        const QKeySequence eventShortcut(ke->keyCombination());

        // Ctrl+F → open search field
        if (eventShortcut == QKeySequence(Qt::CTRL | Qt::Key_F)) {
            searchFieldOpen();
            event->accept();
            return true;
        }

        // Esc → close search field
        if (ke->key() == Qt::Key_Escape) {
            searchFieldClose();
            event->accept();
            return true;
        }
        // When search input is focused:
        if (obj == m_searchInput) {
            // Enter → immediate commit (stop debounce, trigger now)
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                searchFieldCommit();
                event->accept();
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void SchemaBrowserWidget::searchFieldOpen()
{
    m_searchInput->show();
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void SchemaBrowserWidget::searchFieldCommit()
{
    const QString filter = m_searchInput->text();
    m_schemaWidget->rebuildTreeWithFilter(filter);
}

void SchemaBrowserWidget::searchFieldClose()
{
    m_searchInput->hide();
    if (!m_searchInput->text().isEmpty()) {
        m_searchInput->clear();
        m_schemaWidget->rebuildTreeWithFilter(QString());
    }
}
