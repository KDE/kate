/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATESQLPLUGIN_H
#define KATESQLPLUGIN_H

#include <ktexteditor/view.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>

#include <kpluginfactory.h>

class KateSQLPlugin : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit KateSQLPlugin(QObject* parent = nullptr, const QList<QVariant>& = QList<QVariant>());

    ~KateSQLPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override { return 1; };
    KTextEditor::ConfigPage *configPage (int number = 0, QWidget *parent = nullptr) override;
    QString configPageName (int number = 0) const;
    QString configPageFullName (int number = 0) const;
    QIcon configPageIcon (int number = 0) const;

  Q_SIGNALS:
    void globalSettingsChanged();
};

#endif // KATESQLPLUGIN_H

