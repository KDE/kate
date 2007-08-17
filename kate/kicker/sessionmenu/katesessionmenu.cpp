/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2005-2006, Anders Lund <anders@alweb.dk>
 */

#include "katesessionmenu.h"
#include "katesessionmenu.moc"

#include <kconfig.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>
#include <libworkspace/kworkspace.h>

#include <QValidator>

class Validator : public QValidator {
  public:
    Validator( QObject *parent ) : QValidator( parent, 0 ) {}
    ~Validator() {}
    virtual QValidator::State validate( QString &, int & ) const { return QValidator::Acceptable; }
};

K_EXPORT_KICKER_MENUEXT(kate, KateSessionMenu)

KateSessionMenu::KateSessionMenu( QWidget *parent, const QStringList& )
  : KPanelMenu( "", parent ),
    m_parent( parent )
{
}

KateSessionMenu::~KateSessionMenu()
{
}

// update the session list and rebuild the menu
void KateSessionMenu::initialize()
{
  if ( initialized() )
  {
    return;
  }

  m_sessions.clear();

  int id = 0;

  // no session - exec 'kate'
  insertItem( KIcon("kate"), i18n("Start Kate (no arguments)"), id++ );

  // new session - prompt for a name and  exec 'kate --start NAME'
  insertItem( KIcon("new"), i18n("New Kate Session"), id++ );

  // new anonymous session, 'kate --start ""'
  insertItem( KIcon("new"), i18n("New Anonymous Session"), id++ );

  addSeparator();

  QStringList list = KGlobal::dirs()->findAllResources( "data", "kate/sessions/*.katesession", KStandardDirs::NoDuplicates );
  for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
  {
    KConfig _config( *it, KConfig::OnlyLocal );
    KConfigGroup config(&_config, "General" );
    m_sessions.append( config.readEntry( "Name" ) );
  }

  m_sessions.sort();

  for ( QStringList::ConstIterator it1 = m_sessions.begin(); it1 != m_sessions.end(); ++it1 )
  {
    insertItem( *it1, id++ );
  }

  // means for updating, to let the user manually update if he/she added new sessions.
  addSeparator();
  insertItem( KIcon("view-refresh"), i18n("Reload Session List"), this, SLOT(reinitialize()) );
}

void KateSessionMenu::slotExec( int id )
{
  if ( id < 0 )
    return;

  QStringList args;
  if ( id > 0 )
    args << "--start";

  // If a new session is requested we try to ask for a name.
  if ( id == 1 )
  {
    bool ok (false);
    QString name = KInputDialog::getText( i18n("Session Name"),
                                          i18n("Please enter a name for the new session"),
                                          QString(),
                                          &ok, 0, new Validator( m_parent ) );
    if ( ! ok )
      return;

    if ( name.isEmpty() && KMessageBox::questionYesNo( 0,
                          i18n("An unnamed session will not be saved automatically. "
                               "Do you want to create such a session?"),
                          i18n("Create anonymous session?"),
                          KStandardGuiItem::yes(), KStandardGuiItem::cancel(),
                          "kate_session_button_create_anonymous" ) == KMessageBox::No )
      return;

    if ( m_sessions.contains( name ) &&
         KMessageBox::warningYesNo( 0,
                                    i18n("You already have a session named %1. Do you want to open that session?", name ),
                                    i18n("Session exists") ) == KMessageBox::No )
      return;
    else
      // mark the menu as dirty so that it gets rebuild at next display
      // to show the new session
      setInitialized( false );

    args << name;
  }

  else if ( id == 2 )
    args << "";

  else if ( id > 2 )
    args << m_sessions[ id-3 ];

   KToolInvocation::kdeinitExec("kate", args);
}


// kate: space-indent: on; indent-width 2; replace-tabs on;
