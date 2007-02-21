/***************************************************************************
 *   Copyright (C) 2004 by Stephan Möres                                   *
 *   Erdling@gmx.net                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "csnippet.h"

CSnippet::CSnippet(QString sKey, QString sValue, Q3ListViewItem *lvi, QObject *parent, const char *name)
    : QObject(parent, name), _sKey(sKey), _sValue(sValue), _lvi(lvi) {}

CSnippet::~CSnippet() {}

#include "csnippet.moc"
