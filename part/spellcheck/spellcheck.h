/* 
 * Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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

class KateSpellCheckManager : public QObject {
  Q_OBJECT

  typedef QPair<KTextEditor::Range, QString> RangeDictionaryPair;

  public:
    KateSpellCheckManager(QObject* parent = NULL);
    virtual ~KateSpellCheckManager();

      Sonnet::Speller* speller();

      QString defaultDictionary();

  protected:
      QList<QPair<KTextEditor::Range, QString> > spellCheckLanguageRanges(KateDocument *doc, const KTextEditor::Range& range);
      QList<QPair<KTextEditor::Range, QString> > spellCheckWrtHighlightingRanges(KateDocument *doc, const KTextEditor::Range& range,
                                                                                                    const QString& dictionary = QString(),
                                                                                                    bool singleLine = false);
  public:
      QList<QPair<KTextEditor::Range, QString> > spellCheckRanges(KateDocument *doc, const KTextEditor::Range& range,
                                                                                     bool singleLine = false);

    protected:
      Sonnet::Speller *m_speller;
};

#endif
 
// kate: space-indent on; indent-width 2; replace-tabs on;
