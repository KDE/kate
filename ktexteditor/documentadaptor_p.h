/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger  <jowenn@kde.org>

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

#ifndef _ktexteditor_document_h_
#define _ktexteditor_document_h_

#include <QtDBus/QtDBus>

namespace KTextEditor {
	class Document;
	/// For documentation see Document
	class DocumentAdaptor: public QDBusAbstractAdaptor {
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface","org.kde.KTextEditor.Document")
		Q_PROPERTY(const QString& encoding READ encoding WRITE setEncoding)
		Q_PROPERTY(const QString& text READ text WRITE setText)
		Q_PROPERTY(int lines READ lines)
		Q_PROPERTY(bool empty READ isEmpty)
		Q_PROPERTY(int totalCharacters READ totalCharacters)
		public:
			DocumentAdaptor(Document *document);
			virtual ~DocumentAdaptor();
		public Q_SLOTS:
			bool clear();
			bool reload();
			bool save();
			bool saveAs();
			bool setTextLines(const QStringList &text);
			bool isEmpty() const ;
			int lineLength(int line) const;
			QPoint endOfLine(int line) const;
			bool insertText(const QPoint& cursor,const QString& text, bool block);
			bool insertTextLines(const QPoint& cursor,const QStringList& text, bool block);
			bool cursorInText(const QPoint& cursor);
			bool insertLine(int line, const QString& text);
			bool insertLines(int line, const QStringList& text);
			bool removeLine(int line);
#ifdef __GNUC__
#warning "Implement/fix functions with cursor/range parameters / return values"
#warning "fix katepart text manipulation deadlocks"
#endif
		//public:
			bool setEncoding(const QString &encoding);
			const QString &encoding() const;
			bool setText(const QString &text);
			QString text() const;
			int lines() const;
			int totalCharacters() const;
		private:
			Document *m_document;
	};

}

#endif
