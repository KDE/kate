/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateoutputview.h"

#include <KLocalizedString>

#include <QPainter>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>

/**
 * setup text document from data
 */
static void setupText(QTextDocument &doc, const QModelIndex &index)
{
    /**
     * fill in right variant of text
     * we always trim spaces away, to avoid ugly empty line at the start/end
     */
    const auto message = index.data(KateOutputMessageStyledDelegate::MessageRole).toMap();
    if (message.contains(QStringLiteral("plainText"))) {
        doc.setPlainText(message.value(QStringLiteral("plainText")).toString().trimmed());
    } else if (message.contains(QStringLiteral("markDown"))) {
        doc.setMarkdown(message.value(QStringLiteral("markDown")).toString().trimmed());
    } else if (message.contains(QStringLiteral("html"))) {
        doc.setHtml(message.value(QStringLiteral("html")).toString().trimmed());
    }
}

void KateOutputMessageStyledDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    // paint background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else {
        painter->fillRect(option.rect, option.palette.base());
    }

    options.text = QString();
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    painter->translate(option.rect.x(), option.rect.y());

    QTextDocument doc;
    setupText(doc, index);
    doc.drawContents(painter);
}

QSize KateOutputMessageStyledDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const
{
    QTextDocument doc;
    setupText(doc, index);
    return doc.size().toSize();
}

KateOutputView::KateOutputView(KateMainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    // simple vbox layout with just the tree view ATM
    // TODO: e.g. filter and such!
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_messagesTreeView = new QTreeView(this);
    m_messagesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_messagesTreeView->setHeaderHidden(true);
    m_messagesTreeView->setRootIsDecorated(false);
    m_messagesTreeView->setModel(&m_messagesModel);
    layout->addWidget(m_messagesTreeView);

    // we want a special delegate to render the message body, as that might be plain text
    // mark down or HTML
    m_messagesTreeView->setItemDelegateForColumn(1, &m_messageBodyDelegate);
}

void KateOutputView::slotMessage(const QVariantMap &message)
{
    /**
     * type column: shows the type, icons for some types only
     */
    auto typeColumn = new QStandardItem();
    const auto typeString = message.value(QStringLiteral("type")).toString();
    if (typeString == QLatin1String("Error")) {
        typeColumn->setText(i18nc("@info", "Error"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-error")));
    } else if (typeString == QLatin1String("Warning")) {
        typeColumn->setText(i18nc("@info", "Warning"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-warning")));
    } else if (typeString == QLatin1String("Info")) {
        typeColumn->setText(i18nc("@info", "Info"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-information")));
    } else {
        typeColumn->setText(i18nc("@info", "Log"));
    }

    /**
     * body column, formatted text
     * we just set the full message as attribute
     * we have our KateOutputMessageStyledDelegate to render this!
     */
    auto bodyColumn = new QStandardItem();
    bodyColumn->setData(QVariant::fromValue(message), KateOutputMessageStyledDelegate::MessageRole);

    /**
     * add new message to model as one row
     */
    m_messagesModel.appendRow({typeColumn, bodyColumn});
}
