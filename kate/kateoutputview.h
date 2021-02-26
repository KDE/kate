/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_OUTPUT_VIEW_H
#define KATE_OUTPUT_VIEW_H

#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QWidget>

class KateMainWindow;
class QTreeView;

/**
 * Delegate to render the message body.
 */
class KateOutputMessageStyledDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /**
     * the role for the message
     */
    static constexpr int MessageRole = Qt::UserRole + 1;

private:
    /**
     * setup text document from data
     */
    static void setupText(QTextDocument &doc, const QModelIndex &index)
    {
        const auto message = index.data(KateOutputMessageStyledDelegate::MessageRole).toMap();
        if (message.contains(QStringLiteral("plainText"))) {
            doc.setPlainText(message.value(QStringLiteral("plainText")).toString());
        } else if (message.contains(QStringLiteral("markDown"))) {
            doc.setMarkdown(message.value(QStringLiteral("markDown")).toString());
        } else if (message.contains(QStringLiteral("html"))) {
            doc.setHtml(message.value(QStringLiteral("html")).toString());
        }
    }

public:
    KateOutputMessageStyledDelegate() = default;

    void paint(QPainter *painter, const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        QTextDocument doc;
        setupText(doc, index);
        doc.drawContents(painter);
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        QTextDocument doc;
        setupText(doc, index);
        return doc.size().toSize();
    }
};

/**
 * Widget to output stuff e.g. for plugins.
 */
class KateOutputView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct new output, we do that once per main window
     * @param mainWindow parent main window
     * @param parent parent widget (e.g. the tool view in the main window)
     */
    KateOutputView(KateMainWindow *mainWindow, QWidget *parent);

public Q_SLOTS:
    /**
     * slot for incoming messages
     * @param message incoming message we shall handle
     *
     * details of message format:
     *
     * IMPORTANT: at the moment, most stuff is key/value with strings, if the format is more finalized, one can introduce better type for more efficiency/type
     * safety
     *
     * message body, can have different formats, lookup order will be:
     *
     *    message["plainText"] => plain text string to display
     *    message["markDown"] => markdown string to display, we use QTextDocument with MarkdownDialectGitHub
     *    message["html"] => HTML string to display, we use QTextDocument, that means only a subset of HTML https://doc.qt.io/qt-5/richtext-html-subset.html
     *
     * the first thing found will be used.
     *
     * message type, we support at the moment
     *
     *    message["type"] = "Error"
     *    message["type"] = "Warning"
     *    message["type"] = "Info"
     *    message["type"] = "Log"
     *
     * this is take from https://microsoft.github.io/language-server-protocol/specification#window_showMessage MessageType of LSP
     *
     * will lead to appropriate icons/... in the output view
     *
     */
    void slotMessage(const QVariantMap &message);

private:
    /**
     * the main window we belong to
     * each main window has exactly one KateOutputView
     */
    KateMainWindow *const m_mainWindow = nullptr;

    /**
     * Internal tree view to display the messages we get
     */
    QTreeView *m_messagesTreeView = nullptr;

    /**
     * Our message model, at the moment a standard item model
     */
    QStandardItemModel m_messagesModel;

    /**
     * Our special output delegate for the message body
     */
    KateOutputMessageStyledDelegate m_messageBodyDelegate;
};

#endif
