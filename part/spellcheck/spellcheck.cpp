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
#include "katehighlight.h"
#include "katetextline.h"
#include "ontheflycheck.h"
#include "spellcheck.h"

KateSpellCheckManager::KateSpellCheckManager(QObject *parent)
: QObject(parent)
{
  m_speller.restore(KGlobal::config().data());
  m_onTheFlyChecker = new KateOnTheFlyChecker(&m_speller);
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
}

void KateSpellCheckManager::addOnTheFlySpellChecking(KateDocument *doc)
{
  m_onTheFlyChecker->addDocument(doc);
}

void KateSpellCheckManager::removeOnTheFlySpellChecking(KateDocument *doc)
{
  m_onTheFlyChecker->removeDocument(doc);
}

void KateSpellCheckManager::updateOnTheFlySpellChecking(KateDocument *doc)
{
  m_onTheFlyChecker->updateDocument(doc);
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

Sonnet::Speller* KateSpellCheckManager::speller()
{
  return &m_speller;
}

QString KateSpellCheckManager::defaultDictionary() const
{
  return m_speller.defaultLanguage();
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

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckLanguageRanges(KateDocument *doc, const KTextEditor::Range& range)
{
  QString dictionary = doc->dictionary();
  if(dictionary.isEmpty()) {
    dictionary = defaultDictionary();
  }
  QList<RangeDictionaryPair> toReturn;
  toReturn.push_back(RangeDictionaryPair(range, dictionary));
  return toReturn;
}

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckWrtHighlightingRanges(KateDocument *document,
                                                                                         const KTextEditor::Range& range,
                                                                                         const QString& dictionary,
                                                                                         bool singleLine)
{
  QList<QPair<KTextEditor::Range, QString> > toReturn;
  QList<KTextEditor::Range> rangesToSplit;
  if(!singleLine || range.onSingleLine()) {
    rangesToSplit.push_back(range);
  }
  else {
    const int startLine = range.start().line();
    const int startColumn = range.start().column();
    const int endLine = range.end().line();
    const int endColumn = range.end().column();
    for(int line = startLine; line <= endLine; ++line) {
      const int start = (line == startLine) ? startColumn : 0;
      const int end = (line == endLine) ? endColumn : document->lineLength(line);
      KTextEditor::Range toAdd(line, start, line, end);
      if(!toAdd.isEmpty()) {
        rangesToSplit.push_back(toAdd);
      }
    }
  }
  for(QList<KTextEditor::Range>::iterator i = rangesToSplit.begin(); i != rangesToSplit.end(); ++i) {
    KTextEditor::Range rangeToSplit = *i;
    KTextEditor::Cursor begin = KTextEditor::Cursor::invalid();
    const int startLine = rangeToSplit.start().line();
    const int startColumn = rangeToSplit.start().column();
    const int endLine = rangeToSplit.end().line();
    const int endColumn = rangeToSplit.end().column();
    bool inSpellCheckArea = false;
    for(int line = startLine; line <= endLine; ++line) {
      KateTextLine::Ptr kateTextLine = document->kateTextLine(line);
      const int start = (line == startLine) ? startColumn : 0;
      const int end = (line == endLine) ? endColumn : kateTextLine->length();
      const KTextEditor::Cursor startCursor();
      for(int i = start; i < end; ++i) {
        unsigned int attribute = kateTextLine->attribute(i);
        if(!document->highlight()->attributeRequiresSpellchecking(attribute)) {
          if(i == start) {
            continue;
          }
          else if(inSpellCheckArea) {
            toReturn.push_back(RangeDictionaryPair(KTextEditor::Range(begin, KTextEditor::Cursor(line, i)), dictionary));
            begin = KTextEditor::Cursor::invalid();
            inSpellCheckArea = false;
          }
        }
        else if(!inSpellCheckArea) {
          begin = KTextEditor::Cursor(line, i);
          inSpellCheckArea = true;
        }
      }
    }
    if(inSpellCheckArea) {
      toReturn.push_back(RangeDictionaryPair(KTextEditor::Range(begin, rangeToSplit.end()), dictionary));
    }
  }
  return toReturn;
}

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckRanges(KateDocument *doc,
                                                                                   const KTextEditor::Range& range,
                                                                                   bool singleLine)
{
  QList<RangeDictionaryPair> toReturn;
  QList<RangeDictionaryPair> languageRangeList = spellCheckLanguageRanges(doc, range);
  for(QList<RangeDictionaryPair>::iterator i = languageRangeList.begin(); i != languageRangeList.end(); ++i) {
    const RangeDictionaryPair& p = *i;
    toReturn += spellCheckWrtHighlightingRanges(doc, p.first, p.second, singleLine);
  }
  return toReturn;
}

#include "spellcheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
