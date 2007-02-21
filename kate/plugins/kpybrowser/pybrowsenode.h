/***************************************************************************
                          pybrowsenode.h  -  description
                             -------------------
    begin                : Mon Aug 27 2001
    copyright            : (C) 2001 by Christian Bird
    email                : chrisb@lineo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PYBROWSENODE_H
#define PYBROWSENODE_H

#include <q3listview.h>
#include <qpixmap.h>

#define PYCLASS 1
#define PYMETHOD 2
#define PYFUNCTION 3
#define PYVARIABLE 4
#define PYOTHER 5

/**
  *@author Christian Bird
  */



class PyBrowseNode : public Q3ListViewItem  {
public:
	QPixmap *pyClassPixmap;

    PyBrowseNode(Q3ListView *parent, const QString &a_name, const QString &a_signature, int type);
	PyBrowseNode(Q3ListViewItem *parent, const QString &a_name, const QString &a_signature, int type);
	~PyBrowseNode();

    void init(const QString &a_name, const QString &a_signature, int nodeType);

    void setName(const QString &name);
	QString getName()const ;
    void setLine(int line);
    int getLine()const;
    void setSig(const QString &signature);
	QString getSig()const;
    void setType(int type);
    int getType()const;
    void setClass(const QString &a_method_class);
    QString getClass()const;

    QString getQualifiedName()const;

private:
	QString name;
    QString signature;
    QString node_class;
	int line;
	int node_type;

};

#endif
