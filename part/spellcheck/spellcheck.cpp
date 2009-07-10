/* 
   Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)

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

/* If ever threads should be used again, thread communication and
 * synchronization ought to be done with blocking queued signal connections.
 */

#include "spellcheck.h"

#include <QMutex>
#include <QHash>
#include <QTimer>
#include <QThread>

#include <kactioncollection.h>
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/view.h>
#include <sonnet/speller.h>

#include "katedocument.h"
#include "ontheflycheck.h"

KateSpellCheckManager::KateSpellCheckManager(QObject *parent)
: QObject(parent)
{
  m_onTheFlyChecker = new KateOnTheFlyChecker();
  m_onTheFlyChecker->setEnabled(true);
}

KateSpellCheckManager::~KateSpellCheckManager()
{
  stopOnTheFlySpellCheckThread();
  delete m_onTheFlyChecker;
}

void KateSpellCheckManager::onTheFlyCheckDocument(KateDocument *document)
{
  m_onTheFlyChecker->removeDocument(document);
  m_onTheFlyChecker->addDocument(document);
//   m_onTheFlyChecker->textInserted(document, document->documentRange());
}

void KateSpellCheckManager::addOnTheFlySpellChecking(KateDocument *doc)
{
  m_onTheFlyChecker->addDocument(doc);
}

void KateSpellCheckManager::removeOnTheFlySpellChecking(KateDocument *doc)
{
  m_onTheFlyChecker->removeDocument(doc);
}

void KateSpellCheckManager::setOnTheFlySpellCheckEnabled(bool b)
{
  if(b) {
          startOnTheFlySpellCheckThread();
  }
  else {
          stopOnTheFlySpellCheckThread();
  }
}

void KateSpellCheckManager::setOnTheFlySpellCheckEnabled(KateDocument *document, bool b)
{
  m_onTheFlyChecker->updateInstalledSmartRanges(document);
}

void KateSpellCheckManager::createActions(KActionCollection* ac)
{

}

void KateSpellCheckManager::startOnTheFlySpellCheckThread()
{
  kDebug(13000) << "starting spell check thread from thread " << QThread::currentThreadId();
  m_onTheFlyChecker->setEnabled(true);
  onTheFlyCheckOpenDocuments();
}

void KateSpellCheckManager::onTheFlyCheckOpenDocuments()
{
/*
	QHash<KTextEditor::Document*, bool> documentHash;
	const QList<KTextEditor::View*> textViews = m_viewManager->textViews();
	for(QList<KTextEditor::View*>::const_iterator i = textViews.begin();
	    i != textViews.end(); ++i) {
		KTextEditor::Document *document = (*i)->document();
		if(!documentHash.contains(document)) {
			onTheFlyCheckDocument(document);
			documentHash.insert(document, true);
		}
	}
*/
}

void KateSpellCheckManager::stopOnTheFlySpellCheckThread()
{
	m_onTheFlyChecker->setEnabled(false);
	removeOnTheFlyHighlighting();
}

void KateSpellCheckManager::removeOnTheFlyHighlighting()
{
/*
	const QList<KTextEditor::View*> textViews = m_viewManager->textViews();
	foreach ( const KTextEditor::View *view, textViews ) {
		if (!view) {
			continue;
		}
		KTextEditor::Document *document = view->document();
		KTextEditor::SmartInterface *smartInterface =
		             qobject_cast<KTextEditor::SmartInterface*>(document);
		if(smartInterface) {
			smartInterface->smartMutex()->lock();
// 			smartInterface->clearDocumentHighlights();
			const QList<KTextEditor::SmartRange*> highlightsList =
			                                      smartInterface->documentHighlights();
			for(QList<KTextEditor::SmartRange*>::const_iterator j = highlightsList.begin();
			j != highlightsList.end(); ++j) {
				delete(*j);
			}
			smartInterface->smartMutex()->unlock();
		}
	}
*/
}


#include "spellcheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
