/* This file is part of the KDE libraries
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
      [katedocument.cpp, LGPL v2 only]

================RELICENSED=================

    This file is part of the KDE libraries
    Copyright (C) 2008 Joseph Wenninger <jowenn@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

//BEGIN includes
#include "python_encoding.h"
#include "python_encoding.moc"
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
//END includes

K_PLUGIN_FACTORY( KTextEditorPythonEncodingFactory, registerPlugin<KTextEditorPythonEncodingCheck>(); )
K_EXPORT_PLUGIN( KTextEditorPythonEncodingFactory( KAboutData( "ktexteditor_python-encoding", "ktexteditor_plugins", ki18n("PythonEncoding"), "0.1", ki18n("Python Encoding check"), KAboutData::License_LGPL_V2 ) ) )

// kate: space-indent on; indent-width 2; replace-tabs on;
