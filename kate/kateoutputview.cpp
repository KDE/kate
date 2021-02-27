/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateoutputview.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDateTime>
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
    // ensure we recover state
    painter->save();

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

    // ensure we recover state
    painter->restore();
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
    m_messagesTreeView->setAlternatingRowColors(true);
    m_messagesTreeView->setModel(&m_messagesModel);
    layout->addWidget(m_messagesTreeView);

    // we want a special delegate to render the message body, as that might be plain text
    // mark down or HTML
    m_messagesTreeView->setItemDelegateForColumn(3, &m_messageBodyDelegate);

    // read config once
    readConfig();

    // handle config changes
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateOutputView::readConfig);
}

void KateOutputView::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");
    m_showOutputViewForMessageType = cgGeneral.readEntry("Show output view for message type", 1);
}

void KateOutputView::slotMessage(const QVariantMap &message)
{
    /**
     * date time column: we want to know when a message arrived
     * TODO: perhaps store full date time for more stuff later
     */
    auto dateTimeColumn = new QStandardItem();
    const QDateTime current = QDateTime::currentDateTime();
    dateTimeColumn->setText(current.time().toString(Qt::TextDate));

    /**
     * type column: shows the type, icons for some types only
     */
    bool shouldShowOutputToolView = false;
    auto typeColumn = new QStandardItem();
    const auto typeString = message.value(QStringLiteral("type")).toString();
    if (typeString == QLatin1String("Error")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 1);
        typeColumn->setText(i18nc("@info", "Error"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-error")));
    } else if (typeString == QLatin1String("Warning")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 2);
        typeColumn->setText(i18nc("@info", "Warning"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-warning")));
    } else if (typeString == QLatin1String("Info")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 3);
        typeColumn->setText(i18nc("@info", "Info"));
        typeColumn->setIcon(QIcon::fromTheme(QStringLiteral("data-information")));
    } else {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 4);
        typeColumn->setText(i18nc("@info", "Log"));
    }

    /**
     * category
     * provided by sender to better categorize the output into stuff like: lsp, git, ...
     */
    auto categoryColumn = new QStandardItem();
    categoryColumn->setText(message.value(QStringLiteral("category")).toString().trimmed());

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
    m_messagesModel.appendRow({dateTimeColumn, typeColumn, categoryColumn, bodyColumn});

    /**
     * ensure correct sizing
     * OPTIMIZE: we can do that only if e.g. a first time a new type/category pops up
     */
    m_messagesTreeView->resizeColumnToContents(0);
    m_messagesTreeView->resizeColumnToContents(1);
    m_messagesTreeView->resizeColumnToContents(2);

    /**
     * if message requires it => show the tool view if hidden
     */
    if (shouldShowOutputToolView) {
        m_mainWindow->showToolView(parentWidget());
    }
}
