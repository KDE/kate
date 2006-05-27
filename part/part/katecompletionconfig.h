/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#ifndef KATECOMPLETIONCONFIG_H
#define KATECOMPLETIONCONFIG_H

#include <QWidget>

namespace Ui { class CompletionConfigWidget; }

class KateCompletionModel;

/**
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionConfig : public QWidget
{
  Q_OBJECT

  public:
    KateCompletionConfig(KateCompletionModel* model, QWidget* parent = 0L);
    virtual ~KateCompletionConfig();

    void apply();

  private slots:
    void moveColumnUp();
    void moveColumnDown();

  private:
    Ui::CompletionConfigWidget* ui;
    KateCompletionModel* m_model;
};

#endif
