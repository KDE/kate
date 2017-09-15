/* This file is part of the KDE project
 *
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __KATE_SESSION_CHOOSER_H__
#define __KATE_SESSION_CHOOSER_H__

#include <QDialog>

class QPushButton;
class QCheckBox;
class QTreeWidget;
class QTreeWidgetItem;

#include "katesession.h"

class KateSessionChooser : public QDialog
{
    Q_OBJECT

public:
    KateSessionChooser(QWidget *parent, const QString &lastSession);
    ~KateSessionChooser() override;

    KateSession::Ptr selectedSession();
    bool reopenLastSession();

    enum {
        resultQuit = QDialog::Rejected,
        resultOpen,
        resultNew,
        resultNone,
        resultCopy
    };

protected Q_SLOTS:
    void slotCancel();
    void slotOpen();
    void slotNew();
    void slotCopySession();
    void slotDeleteSession();

    /**
     * selection has changed
     */
    void selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    QTreeWidget *m_sessions;
    QCheckBox *m_useLast;
    QPushButton *m_openButton;
};

#endif

