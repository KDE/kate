/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "katefactory.h"
#include "katefactory.moc"

#include "katedocument.h"

#include <klocale.h>
#include <kinstance.h>
#include <kaboutdata.h>

extern "C"
{
  void *init_libkatepart()
  {
    KGlobal::locale()->insertCatalogue("katepart");
    return new KateFactory();
  }
}

KInstance *KateFactory::s_instance = 0L;

KateFactory::KateFactory()
{
  s_instance = 0L;
}

KateFactory::~KateFactory()
{
  if ( s_instance )
  {
    delete s_instance->aboutData();
    delete s_instance;
  }
  s_instance = 0L;
}

KParts::Part *KateFactory::createPartObject( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList & )
{
  bool bWantSingleView = !( classname == QString("KTextEditor::Document") );
  bool bWantBrowserView = ( classname == QString("Browser/View") );
  bool bWantReadOnly = (bWantBrowserView || ( classname == QString("KParts::ReadOnlyPart") ));

  KParts::ReadWritePart *part = new KateDocument (bWantSingleView, bWantBrowserView, bWantReadOnly, parentWidget, widgetName, parent, name);
  part->setReadWrite( !bWantReadOnly );

  return part;
}

KInstance *KateFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( aboutData() );
  return s_instance;
}

const KAboutData *KateFactory::aboutData()
{
  KAboutData *data = new KAboutData  ("kate", I18N_NOOP("Kate"), "2.0",
                                                           I18N_NOOP( "Kate - KDE Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                                                           I18N_NOOP( "(c) 2000-2001 The Kate Authors" ), 0, "http://kate.sourceforge.net");

  data->addAuthor ("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "cullmann@kde.org", "http://www.babylon2k.de");
  data->addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  data->addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  data->addAuthor ("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
  data->addAuthor ("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
  data->addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  data->addAuthor ("Charles Samuels", I18N_NOOP("Core Developer"), "charles@kde.org");
  data->addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  data->addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  data->addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  data->addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  data->addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  data->addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  data->addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  data->addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  data->addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

  data->addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  data->addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  data->addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  data->addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  data->addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  data->addCredit ("John Firebaugh",I18N_NOOP("Highlighting for Java, and much more"),"");
  data->addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  data->addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  data->addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  data->addCredit ("Daniel Naber","","");
  data->addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  data->addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  data->addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
  data->addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
  data->addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  data->addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  data->setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  return data;
}
