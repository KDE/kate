/***************************************************************************
                         attribeditor.h -  description
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
#ifndef ATTRIB_EDITOR
#define ATTRIB_EDITOR

#include "katehledit_attrib_skel.h"

class AttribEditor: public AttribEditor_skel
{
	Q_OBJECT
public:
	AttribEditor(QWidget *parent);
	~AttribEditor();
	void load(class SyntaxDocument *);
	class QStringList attributeNames();
public slots:
	void currentAttributeChanged(QListViewItem *item);
	void slotAddAttribute();
	void updateAttributeName(const QString &);
	void updateAttributeType(const QString &);
	void updateAttributeColour(const QColor &);
	void updateAttributeSelectedColour(const QColor &);
};

#endif