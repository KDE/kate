/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "document.h"
#include "document.moc"

using namespace KTextEditor;

class KTextEditor::DocumentPrivate {
  public:
    DocumentPrivate()
      : openingError(false), suppressOpeningErrorDialogs(false) { }
    bool openingError;
    bool suppressOpeningErrorDialogs;
    QString openingErrorMessage;
};

Document::Document( QObject *parent)
 : KParts::ReadWritePart(parent)
 , d(new DocumentPrivate())
{
}

Document::~Document()
{
  delete d;
}

void Document::setSuppressOpeningErrorDialogs(bool suppress) {
  d->suppressOpeningErrorDialogs=suppress;
}

bool Document::suppressOpeningErrorDialogs() const {
  return d->suppressOpeningErrorDialogs;
}

bool Document::openingError() const {
  return d->openingError;
}

QString Document::openingErrorMessage() const {
  return d->openingErrorMessage;
}

void Document::setOpeningError(bool errors) {
  d->openingError=errors;
}

void Document::setOpeningErrorMessage(const QString& message) {
  d->openingErrorMessage=message;
}

bool Document::cursorInText(const Cursor& cursor)
{
  if ( (cursor.line()<0) || (cursor.line()>=lines())) return false;
  return (cursor.column()>=0) && (cursor.column()<=lineLength(cursor.line())); // = because new line isn't usually contained in line length
}

bool KTextEditor::Document::replaceText( const Range & range, const QString & text, bool block )
{
  bool success = true;
  startEditing();
  success &= removeText(range, block);
  success &= insertText(range.start(), text, block);
  endEditing();
  return success;
}

bool Document::replaceText( const Range & range, const QStringList & text, bool block )
{
  bool success = true;
  startEditing();
  success &= removeText(range, block);
  success &= insertText(range.start(), text, block);
  endEditing();
  return success;
}

bool Document::isEmpty( ) const
{
  return documentEnd() == Cursor::start();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
