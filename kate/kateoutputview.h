/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __KATE_OUTPUT_VIEW_H__
#define __KATE_OUTPUT_VIEW_H__

#include <QStandardItemModel>
#include <QWidget>

class KateMainWindow;
class QTreeView;

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
};

#endif
