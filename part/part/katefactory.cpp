/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   
   Based on KHTML Factory from Simon Hausmann <hausmann@kde.org>

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
#include "kateview.h"

#include <klocale.h>
#include <kinstance.h>
#include <kaboutdata.h>

#include <assert.h>

template class QPtrList<KateDocument>;
template class QPtrList<KateView>;

KateFactory *KateFactory::s_self = 0;
unsigned long int KateFactory::s_refcnt = 0;
KInstance *KateFactory::s_instance = 0;
KAboutData *KateFactory::s_about = 0;
QPtrList<KateDocument> *KateFactory::s_documents = 0;
QPtrList<KateView> *KateFactory::s_views = 0;
KTrader::OfferList *KateFactory::s_plugins = 0; 

extern "C"
{
  void *init_libkatepart()
  {
    return new KateFactory( true );
  }
}

KateFactory::KateFactory( bool clone )
{
  if ( clone )
  {
    ref();
    return;
  }
}

KateFactory::~KateFactory()
{
  if ( s_self == this )
  {
        assert( !s_refcnt );

        if ( s_instance )
            delete s_instance;
            
        if ( s_about )
            delete s_about;
            
        if ( s_documents )
        {
            assert( s_documents->isEmpty() );
            delete s_documents;
        }
            
        if ( s_views )
        {
            assert( s_views->isEmpty() );
            delete s_views;
        }
	
	if ( s_plugins )
	  delete s_plugins;
	          
        s_instance = 0;
        s_about = 0;
        s_documents = 0;
        s_views = 0;
	s_plugins = 0;
    }
    else
        deref();
}

void KateFactory::ref()
{
    if ( !s_refcnt && !s_self )
    {
      s_self = new KateFactory;
    }

    s_refcnt++;
}

void KateFactory::deref()
{
    if ( !--s_refcnt && s_self )
    {
        delete s_self;
        s_self = 0;
    }
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

void KateFactory::registerDocument ( KateDocument *doc )
{
    if ( !s_documents )
        s_documents = new QPtrList<KateDocument>;

    if ( !s_documents->containsRef( doc ) )
    {
        s_documents->append( doc );
        ref();
    }
}

void KateFactory::deregisterDocument ( KateDocument *doc )
{
    assert( s_documents );

    if ( s_documents->removeRef( doc ) )
    {
        if ( s_documents->isEmpty() )
        {
            delete s_documents;
            s_documents = 0;
        }
        
        deref();
    }
}

void KateFactory::registerView ( KateView *view )
{
    if ( !s_views )
        s_views = new QPtrList<KateView>;

    if ( !s_views->containsRef( view ) )
    {
        s_views->append( view );
        ref();
    }
}

void KateFactory::deregisterView ( KateView *view )
{
    assert( s_views );

    if ( s_views->removeRef( view ) )
    {
        if ( s_views->isEmpty() )
        {
            delete s_views;
            s_views = 0;
        }
        
        deref();
    }
}

KTrader::OfferList *KateFactory::plugins ()
{
  if ( !s_plugins )
   s_plugins = new QValueList<KService::Ptr> (KTrader::self()->query("KTextEditor/Plugin"));
   
  return s_plugins;
}

KInstance *KateFactory::instance()
{
  assert( s_self );

  if ( !s_instance )
  {
    s_about = new KAboutData  ("katepart", I18N_NOOP("Kate Part"), "2.0",
                                                           I18N_NOOP( "Embeddable editor component" ), KAboutData::License_LGPL_V2,
                                                           I18N_NOOP( "(c) 2000-2002 The Kate Part Authors" ), 0, "http://kate.kde.org");

    s_about->addAuthor ("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "cullmann@kde.org", "http://www.babylon2k.de");
    s_about->addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
    s_about->addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
    s_about->addAuthor ("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
    s_about->addAuthor ("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
    s_about->addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
    s_about->addAuthor ("Charles Samuels", I18N_NOOP("Core Developer"), "charles@kde.org");
    s_about->addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
    s_about->addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
    s_about->addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
    s_about->addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
    s_about->addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
    s_about->addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
    s_about->addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
    s_about->addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
    s_about->addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

    s_about->addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
    s_about->addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
    s_about->addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
    s_about->addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
    s_about->addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
    s_about->addCredit ("John Firebaugh",I18N_NOOP("Highlighting for Java, and much more"),"");
    s_about->addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
    s_about->addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
    s_about->addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
    s_about->addCredit ("Daniel Naber","","");
    s_about->addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
    s_about->addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
    s_about->addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
    s_about->addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
    s_about->addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
    s_about->addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

    s_about->setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

    s_instance = new KInstance( s_about );
  }

  return s_instance;
}
