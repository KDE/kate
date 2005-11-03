/* This file is part of the KDE libraries
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

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
#ifndef _KATE_TEMPLATE_HANDLER_H_
#define _KATE_TEMPLATE_HANDLER_H_

#include "katesmartrange.h"
#include "katekeyinterceptorfunctor.h"
#include <qobject.h>
#include <qmap.h>
#include <QHash>
#include <q3ptrlist.h>
#include <qstring.h>
#include <QList>

class KateDocument;

class KateTemplateHandler: public QObject, public KateKeyInterceptorFunctor {
		Q_OBJECT
	public:
		KateTemplateHandler(KateDocument *doc,const KTextEditor::Cursor& position, const QString &templateString, const QMap<QString,QString> &initialValues);
		virtual ~KateTemplateHandler();
		inline bool initOk() {return m_initOk;}
		virtual bool operator()(KKey key);
	private:
		struct KateTemplatePlaceHolder {
      KateTemplatePlaceHolder(KateDocument* doc) {}
			bool isCursor;
			bool isInitialValue;
		};
		class KateTemplateHandlerPlaceHolderInfo{
			public:
				KateTemplateHandlerPlaceHolderInfo():begin(0),len(0),placeholder(""){}
				KateTemplateHandlerPlaceHolderInfo(uint begin_,uint len_,const QString& placeholder_):begin(begin_),len(len_),placeholder(placeholder_){}
				uint begin;
				uint len;
				QString placeholder;
		};
		class KateRangeList *m_ranges;
		class KateDocument *m_doc;
		Q3PtrList<KateTemplatePlaceHolder> m_tabOrder;

                // looks like this is leaking objects (was before too)
                QHash<QString, KateTemplatePlaceHolder*> m_dict;

		void generateRangeTable(const KTextEditor::Cursor& insertPosition, const QString& insertString, const QList<KateTemplateHandlerPlaceHolderInfo> &buildList);
		int m_currentTabStop;
		KateSmartRange *m_currentRange;
		void locateRange(const KTextEditor::Cursor &cursor );
		bool m_initOk;
		bool m_recursion;
	private slots:
		void slotTextInserted(int,int);
		void slotDocumentDestroyed();
		void slotAboutToRemoveText(const KTextEditor::Range& range);
		void slotTextRemoved();
};
#endif
