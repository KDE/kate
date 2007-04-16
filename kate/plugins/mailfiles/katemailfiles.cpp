/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katemailfiles.h"
#include "katemailfiles.moc"

#include "katemailfilesdialog.h"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <KToolInvocation>

#include <kgenericfactory.h>
#include <kauthorized.h>

K_EXPORT_COMPONENT_FACTORY( katemailfilesplugin, KGenericFactory<KateMailFilesPlugin>( "katemailfilesplugin" ) )

KateMailFilesPlugin::KateMailFilesPlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{}

Kate::PluginView *KateMailFilesPlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateMailFilesPluginView (mainWindow);
}

KateMailFilesPluginView::KateMailFilesPluginView (Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow)
{
  actionCollection()->addAction( KStandardAction::Mail, this, SLOT(slotMail()) )
  ->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));
  setComponentData (KComponentData("kate"));
  setXMLFile("plugins/katemailfiles/ui.rc");
  mainWindow->guiFactory()->addClient (this);
}

KateMailFilesPluginView::~KateMailFilesPluginView ()
{}

void KateMailFilesPluginView::slotMail()
{
  KateMailDialog *d = new KateMailDialog(mainWindow()->window(), mainWindow());
  if ( ! d->exec() )
  {
    delete d;
    return;
  }
  QList<KTextEditor::Document *> attDocs = d->selectedDocs();
  delete d;
  // Check that all selected files are saved (or shouldn't be)
  QStringList urls; // to atthatch
  KTextEditor::Document *doc;
  for ( QList<KTextEditor::Document *>::iterator it = attDocs.begin();
        it != attDocs.end(); ++it )
  {
    doc = *it;
    if (!doc) continue;
    if ( doc->url().isEmpty() )
    {
      // unsaved document. back out unless it gets saved
      int r = KMessageBox::questionYesNo( mainWindow()->window(),
                                          i18n("<p>The current document has not been saved, and "
                                               "cannot be attached to an email message."
                                               "<p>Do you want to save it and proceed?"),
                                          i18n("Cannot Send Unsaved File"), KStandardGuiItem::saveAs(), KStandardGuiItem::cancel() );
      if ( r == KMessageBox::Yes )
      {
        bool sr = doc->documentSaveAs();
        /* if ( sr == KTextEditor::View::SAVE_OK ) { ;
         }
         else {*/
        if ( !sr  ) // ERROR or RETRY(?)
        {   KMessageBox::sorry( mainWindow()->window(), i18n("The file could not be saved. Please check "
                                  "if you have write permission.") );
          continue;
        }
      }
      else
        continue;
    }
    if ( doc->isModified() )
    {
      // warn that document is modified and offer to save it before proceeding.
      int r = KMessageBox::warningYesNoCancel( mainWindow()->window(),
              i18n("<p>The current file:<br><strong>%1</strong><br>has been "
                   "modified. Modifications will not be available in the attachment."
                   "<p>Do you want to save it before sending it?", doc->url().prettyUrl()),
              i18n("Save Before Sending?"), KStandardGuiItem::save(), KGuiItem(i18n("Do Not Save")) );
      switch ( r )
      {
        case KMessageBox::Cancel:
          continue;
        case KMessageBox::Yes:
          doc->save();
          if ( doc->isModified() )
          { // read-only docs ends here, if modified. Hmm.
            KMessageBox::sorry( mainWindow()->window(), i18n("The file could not be saved. Please check "
                                "if you have write permission.") );
            continue;
          }
          break;
        default:
          break;
      }
    }
    // finally call the mailer
    urls << doc->url().url();
  } // check selected docs done
  if ( ! urls.count() )
    return;
  KToolInvocation::invokeMailer( QString(), // to
                                 QString(), // cc
                                 QString(), // bcc
                                 QString(), // subject
                                 QString(), // body
                                 QString(), // msgfile
                                 urls           // urls to atthatch
                               );
}
// kate: space-indent on; indent-width 2; replace-tabs on;

