/***************************************************************************
                          katefactory.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __kate_factory_h__
#define __kate_factory_h__

#include "kateglobal.h"

#include <kparts/factory.h>

class KInstance;
class KAboutData;

class KateFactory : public KParts::Factory
{
  Q_OBJECT
public:
  KateFactory();
  virtual ~KateFactory();

  virtual KParts::Part *createPartObject( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList &args );
  virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

  static const KAboutData *aboutData();
  static KInstance *instance();

private:
  static KInstance *s_instance;
  static KAboutData *s_aboutData;
};

#endif
