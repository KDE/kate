/***************************************************************************
                         attribeditor.cpp -  description
                             -------------------
    begin                : Fri Jan 11 2002
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation;

     =======================================
     ONLY VERSION 2 OF THE LICENSE IS VALID
     =======================================

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/
#include "attribeditor.h"
#include "attribeditor.moc"
#include "katesyntaxdocument.h"

#include <kcombobox.h>
#include <klistview.h>
#include <klineedit.h>
#include <kcolorcombo.h>
#include <qcheckbox.h>
#include <klocale.h>
#include <qstringlist.h>
#include <qpushbutton.h>

AttribEditor::AttribEditor(QWidget *parent):AttribEditor_skel(parent)
{
	attributes->setSorting(-1);
	AttributeType->insertItem("dsNormal");
	AttributeType->insertItem("dsKeyword");
	AttributeType->insertItem("dsDataType");
	AttributeType->insertItem("dsDecVal");
	AttributeType->insertItem("dsBaseN");
	AttributeType->insertItem("dsFloat");
	AttributeType->insertItem("dsChar");
	AttributeType->insertItem("dsString");
	AttributeType->insertItem("dsComment");
	AttributeType->insertItem("dsOthers");
	AttributeType->insertItem(i18n("Custom"));
	connect(attributes,SIGNAL(currentChanged(QListViewItem*)),this,SLOT(currentAttributeChanged(QListViewItem*)));
	connect(addAttribute,SIGNAL(clicked()),this,SLOT(slotAddAttribute()));
	connect(AttributeName,SIGNAL(textChanged(const QString&)),this,SLOT(updateAttributeName(const QString&)));
}

AttribEditor::~AttribEditor()
{
}

void AttribEditor::load(SyntaxDocument*doc )
{
  struct syntaxContextData *data;
  data=doc->getGroupInfo("highlighting","itemData");
  int cnt=0;
  QListViewItem *prev=0;
  while (doc->nextGroup(data))
    {
//        ContextAttribute->insertItem(HlManager::self()->syntax->groupData(data,QString("name")));
//        ItemAttribute->insertItem(HlManager::self()->syntax->groupData(data,QString("name")));
	attributes->insertItem(prev=new QListViewItem(attributes,prev,
		doc->groupData(data,QString("name")),
		doc->groupData(data,QString("defStyleNum")),
		doc->groupData(data,QString("color")),
		doc->groupData(data,QString("selColor")),
		doc->groupData(data,QString("bold")),
		doc->groupData(data,QString("italic")),
		QString("%1").arg(cnt)));
	cnt++;
    }
  if (data) doc->freeGroupInfo(data);
  currentAttributeChanged(attributes->firstChild());
}

QStringList AttribEditor::attributeNames()
{
	QStringList list;
	for (QListViewItem *it=attributes->firstChild(); it!=0; it=it->nextSibling())
	{
		list<<it->text(0);
	}
	return list;
}

void AttribEditor::slotAddAttribute()
{
	attributes->insertItem(new QListViewItem(attributes,attributes->lastItem(),
		i18n("New attribute"),"dsNormal","#000000","#ffffff","0","0",
		QString("%1").arg(attributes->childCount())));
}

void AttribEditor::currentAttributeChanged(QListViewItem *item)
{
	if (item)
	{
		bool isCustom=(item->text(1)=="dsNormal")&&(!(item->text(2).isEmpty()));
		AttributeName->setText(item->text(0));
		AttributeType->setCurrentText(
			isCustom?i18n("Custom"):item->text(1));
		AttributeName->setEnabled(true);
		AttributeType->setEnabled(true);
		if (isCustom)
		{
			Colour->setColor(QColor(item->text(2)));
			SelectedColour->setColor(QColor(item->text(3)));
			Bold->setChecked(item->text(4)=="1");
			Italic->setChecked(item->text(5)=="1");

			Colour->setEnabled(true);
			SelectedColour->setEnabled(true);
			Bold->setEnabled(true);
			Italic->setEnabled(true);
		}
		else
		{
			Colour->setEnabled(false);
			SelectedColour->setEnabled(false);
			Bold->setEnabled(false);
			Italic->setEnabled(false);
		}
	}
	else
	{
		Colour->setEnabled(false);
		SelectedColour->setEnabled(false);
		Bold->setEnabled(false);
		Italic->setEnabled(false);
		AttributeName->setEnabled(false);
		AttributeType->setEnabled(false);
	}
}


void AttribEditor::updateAttributeName(const QString &text)
{
	if (attributes->currentItem())
	{
		attributes->currentItem()->setText(0,text);
	}
}