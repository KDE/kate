/***************************************************************************
 *   Copyright (C) 2004 by Stephan Möres                                   *
 *   Erdling@gmx.net                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef CSNIPPET_H
#define CSNIPPET_H

#include <qobject.h>
#include <q3listview.h>
#include <kaction.h>
#include <qsignalmapper.h>

/**
@author Stephan Möres
*/

class CSnippet : public QObject {
  Q_OBJECT
public:
  CSnippet(QString sKey, QString sValue, Q3ListViewItem *lvi, QObject *parent = 0, const char *name = 0);
  ~CSnippet();
  QString getKey() 						{ return _sKey; };
  QString getValue()						{ return _sValue; };
  Q3ListViewItem* getListViewItem() const	{ return _lvi; };
  void setKey(const QString& sKey) 				{ _sKey = sKey; };
  void setValue(const QString& sValue) 			{ _sValue = sValue; };

protected:
  QString            _sKey;
  QString            _sValue;
  Q3ListViewItem      *_lvi;
};

#endif
