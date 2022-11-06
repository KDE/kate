/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_OUTPUT_VIEW_H
#define KATE_OUTPUT_VIEW_H

#include <QLineEdit>
#include <QPointer>
#include <QTextBrowser>
#include <QTimer>
#include <QWidget>

class KateMainWindow;
class KateOutputTreeView;
class QSortFilterProxyModel;

/**
 * Widget to output stuff e.g. for plugins.
 */
class KateOutputView : public QWidget
{
    Q_OBJECT

public:
    enum Column {
        Column_Time = 0,
        Column_Category,
        Column_LogType,
        Column_Body,
    };
    /**
     * Construct new output, we do that once per main window
     * @param mainWindow parent main window
     * @param parent parent widget (e.g. the tool view in the main window)
     */
    KateOutputView(KateMainWindow *mainWindow, QWidget *parent, QWidget *tabButton);

public Q_SLOTS:
    /**
     * Read and apply our configuration.
     */
    void readConfig();

    /**
     * slot for incoming messages
     * @param message incoming message we shall handle
     *
     * details of message format:
     *
     * IMPORTANT: at the moment, most stuff is key/value with strings, if the format is more finalized, one can introduce better type for more efficiency/type
     * safety
     *
     * message text, will be trimmed before output
     *
     *    message["text"] = i18n("your cool message")
     *
     * the text will be split in lines, all lines beside the first can be collapsed away
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
     * a message should have some category, like Git, LSP, ....
     *
     *    message["category"] = i18n(...)
     *
     * will be used to allow the user to filter for
     *
     * one can additionally provide a categoryIcon
     *
     *    message["categoryIcon"] = QIcon(...)
     *
     * the categoryIcon icon QVariant must contain a QIcon, nothing else!
     *
     */
    void slotMessage(const QVariantMap &message);

private:
    void search();
    void appendLines(const QStringList &lines, const QString &token, const QTextCursor &pos = {});

    /**
     * the main window we belong to
     * each main window has exactly one KateOutputView
     */
    KateMainWindow *const m_mainWindow = nullptr;

    // The text edit used to display messages
    class KateOutputEdit *m_textEdit;

    /**
     * fuzzy filter line edit
     */
    QLineEdit m_filterLine;

    /**
     * When to show output view
     * 0 => never
     * 1 => on error
     * 2 => on warning or above
     * 3 => on info or above
     * 4 => on log or above
     */
    int m_showOutputViewForMessageType = 1;

    /**
     * history size limit, < 0 is unlimited
     */
    int m_historyLimit = -1;

    QPointer<QWidget> tabButton;

    QPointer<class NewMsgIndicator> m_fadingIndicator;

    QColor m_msgIndicatorColors[3];

    QTimer m_searchTimer;

    QString m_infoColor;
    QString m_warnColor;
    QString m_errColor;
    QString m_keywordColor;
};

#endif
