/***************************************************************************
 *   Copyright (C) 2004 by Stephan Möres                                   *
 *   Erdling@gmx.net                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef CWIDGETSNIPPETS_H
#define CWIDGETSNIPPETS_H

#include <CWidgetSnippetsBase.h>

/**
@author Stephan Möres
*/
class CWidgetSnippets : public CWidgetSnippetsBase {
public:
  CWidgetSnippets( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );

  ~CWidgetSnippets();

};

#endif
