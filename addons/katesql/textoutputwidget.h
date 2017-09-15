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

#ifndef TEXTOUTPUTWIDGET_H
#define TEXTOUTPUTWIDGET_H

class QHBoxLayout;
class QTextEdit;

#include "connection.h"
#include <qwidget.h>

class TextOutputWidget : public QWidget
{
  Q_OBJECT

  public:
    TextOutputWidget(QWidget *parent = nullptr);
    ~TextOutputWidget() override;

  public Q_SLOTS:
    void showErrorMessage(const QString &message);
    void showSuccessMessage(const QString &message);

  private:
    void writeMessage(const QString &msg);

  private:
    QHBoxLayout *m_layout;
    QTextEdit *m_output;

    QColor m_succesTextColor;
    QColor m_succesBackgroundColor;
    QColor m_errorTextColor;
    QColor m_errorBackgroundColor;
};

#endif // TEXTOUTPUTWIDGET_H
