/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "hlparamedit.h"
#include "hlparamedit.moc"

#include <klocale.h>

#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

HLParamEdit::HLParamEdit(QWidget *parent):QHBox(parent)
{
  textEdit=0;
  listLabel=0;
  listChoose=0;
  listNew=0;
  listEdit=0;
}

HLParamEdit::~HLParamEdit(){;}

void HLParamEdit::ListParameter(QString listname)
{
  delete textEdit;
  textEdit=0;

  if (!listLabel)
  {
    listLabel=new QLabel(listname,this);
    listChoose=new QPushButton(i18n("Choose"),this);
    listNew=new QPushButton(i18n("New"),this);
    listEdit=new QPushButton(i18n("Edit"),this);
  }
  listLabel->setText(listname);
  listLabel->show();
  listChoose->show();
  listNew->show();
  listEdit->show();
}

void HLParamEdit::TextParameter(int length, QString text, bool /* regExp */)
{
  delete listLabel;
  delete listChoose;
  delete listNew;
  delete listEdit;
  listLabel=0;
  listChoose=0;
  listNew=0;
  listEdit=0;

  if (!textEdit)
  {
    textEdit=new QLineEdit(this);
    connect(textEdit,SIGNAL(textChanged(const QString&)),this,SIGNAL(textChanged(const QString&)));
  }
  textEdit->setMaxLength(length);
  textEdit->setText(text);
  textEdit->show();
}

QString HLParamEdit::text()
{
  if (!textEdit)
     return QString::null;

  return textEdit->text();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
