/***************************************************************************
                          hlparamedit.cpp  -  description
                             -------------------
    begin                : Sat 31 March 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
