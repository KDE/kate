#include "hlparamedit.h"
#include "hlparamedit.moc"

#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <klocale.h>

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

void HLParamEdit::TextParameter(int length, QString text,bool regExp)
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
