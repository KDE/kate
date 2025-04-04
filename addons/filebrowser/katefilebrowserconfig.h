/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>
   SPDX-FileCopyrightText: 2009 Dominik Haumann <dhaumann kde org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <ktexteditor/configpage.h>

class KateFileBrowser;
class KActionSelector;

class KateFileBrowserConfigPage : public KTextEditor::ConfigPage
{
public:
    explicit KateFileBrowserConfigPage(QWidget *parent = nullptr, KateFileBrowser *kfb = nullptr);
    ~KateFileBrowserConfigPage() override
    {
    }

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset() override;
    void defaults() override
    {
    }

private Q_SLOTS:
    void slotMyChanged();

private:
    void init();

    KateFileBrowser *fileBrowser;
    KActionSelector *acSel;

    bool m_changed = false;
};
