/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2007 Anders Lund <anders@alweb.dk>
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_FILETREE_CONFIGPAGE_H
#define KATE_FILETREE_CONFIGPAGE_H

#include <QWidget>

#include <ktexteditor/configpage.h>

class KateFileTreePlugin;

class KateFileTreeConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT
public:
    explicit KateFileTreeConfigPage(QWidget *parent = nullptr, KateFileTreePlugin *plug = nullptr);
    ~KateFileTreeConfigPage() override {}

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void defaults() override;
    void reset() override;

    //Q_SIGNALS:
    //  void changed();

private Q_SLOTS:
    void slotMyChanged();

private:
    class QGroupBox *gbEnableShading;
    class KColorButton *kcbViewShade, *kcbEditShade;
    class QLabel *lEditShade, *lViewShade, *lSort, *lMode;
    class KComboBox *cmbSort, *cmbMode;
    class QCheckBox *cbShowFullPath;
    KateFileTreePlugin *m_plug;

    bool m_changed;
};

#endif /* KATE_FILETREE_CONFIGPAGE_H */

