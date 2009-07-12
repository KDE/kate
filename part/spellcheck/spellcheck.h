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

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

#include <ktexteditor/document.h>
#include <sonnet/backgroundchecker.h>
#include <sonnet/speller.h>

class KateDocument;
class KateView;
class KateOnTheFlyChecker;

class KateSpellCheckManager : public QObject {
  Q_OBJECT

  typedef QPair<KTextEditor::Range, QString> RangeDictionaryPair;

  public:
    KateSpellCheckManager(QObject* parent = NULL);
    virtual ~KateSpellCheckManager();

      void onTheFlyCheckDocument(KateDocument *document);
      void addOnTheFlySpellChecking(KateDocument *doc);
      void removeOnTheFlySpellChecking(KateDocument *doc);
      void updateOnTheFlySpellChecking(KateDocument *doc);

      void setOnTheFlySpellCheckEnabled(KateDocument *document, bool b);

      void createActions(KActionCollection* ac);

      Sonnet::Speller* speller();

      QString defaultDictionary() const;

      QList<QPair<KTextEditor::Range, QString> > spellCheckLanguageRanges(KateDocument *doc, const KTextEditor::Range& range);
      QList<QPair<KTextEditor::Range, QString> > spellCheckWrtHighlightingRanges(KateDocument *doc, const KTextEditor::Range& range,
                                                                                                    const QString& dictionary = QString(),
                                                                                                    bool singleLine = false);
      QList<QPair<KTextEditor::Range, QString> > spellCheckRanges(KateDocument *doc, const KTextEditor::Range& range,
                                                                                     bool singleLine = false);

    public Q_SLOTS:
      void setOnTheFlySpellCheckEnabled(bool b);

    protected:
      Sonnet::Speller m_speller;
      KateOnTheFlyChecker *m_onTheFlyChecker;

      void startOnTheFlySpellCheckThread();
      void stopOnTheFlySpellCheckThread();
      void removeOnTheFlyHighlighting();
      void onTheFlyCheckOpenDocuments();
};

#endif
 
// kate: space-indent on; indent-width 2; replace-tabs on;
