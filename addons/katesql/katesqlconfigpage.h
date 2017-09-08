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

#ifndef KATESQLCONFIGPAGE_H
#define KATESQLCONFIGPAGE_H

class OutputStyleWidget;
class QCheckBox;

#include "katesqlplugin.h"

#include <ktexteditor/configpage.h>

/// TODO: add options to change datetime and numbers format

class KateSQLConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  public:
    explicit KateSQLConfigPage( QWidget* parent = nullptr );
    virtual ~KateSQLConfigPage();

    QString name() const Q_DECL_OVERRIDE;
    QString fullName() const Q_DECL_OVERRIDE;
    QIcon icon() const Q_DECL_OVERRIDE;

  public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

  private:
    KateSQLPlugin *m_plugin;
    QCheckBox *m_box;
    OutputStyleWidget *m_outputStyleWidget;

  Q_SIGNALS:
    void settingsChanged();
};

#endif // KATESQLCONFIGPAGE_H

