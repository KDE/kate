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

#ifndef _HL_PARAM_EDIT_H_
#define _HL_PARAM_EDIT_H_

#include "kateglobal.h"

#include <qhbox.h>

class HLParamEdit:public QHBox
{
Q_OBJECT
public:
	HLParamEdit(QWidget *parent);
	~HLParamEdit();
	void ListParameter(QString listname);
	void TextParameter(int length, QString text,bool regExp=false);
	QString text();
private:
	class QLineEdit *textEdit;
	class QLabel *listLabel;
	class QPushButton *listChoose;
	class QPushButton *listNew;
	class QPushButton *listEdit;
/*private slots:
	void listEditClicked();
	void listNewClicked();
	void listChooseClicked();*/
signals:
	void textChanged(const QString&);
};
#endif
