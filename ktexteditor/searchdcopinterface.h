#ifndef SEARCH_DCOP_INTERFACE_H
#define SEARCH_DCOP_INTERFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <qstringlist.h>
#include <qcstring.h>

namespace KTextEditor
{
	class SearchInterface;
	/**
	This is the main interface to the @ref SearchInterface of KTextEdit.
	This will provide a consistant dcop interface to all KDE applications that use it.
	@short DCOP interface to @ref SearchInterface.
	@author Ian Reinhart Geiser <geiseri@kde.org>
	*/
	class SearchDCOPInterface : virtual public DCOPObject
	{
	K_DCOP

	public:
		/**
		Construct a new interface object for the text editor.
		@param ParentSearchInterface - The parent @ref SearchInterface object
		that will provide us with the functions for the interface.
		*/
		SearchDCOPInterface( SearchInterface *Parent, const char *name );
		/**
		Destructor
		Cleans up the object.
		**/
		virtual ~SearchDCOPInterface();
	k_dcop:
		bool findFirstString(QString text, bool caseSensitive);
		bool findNextString(QString text, bool caseSensitive);
		bool findPreviousString( QString text, bool caseSensitive);
		bool findLastString(QString text, bool caseSensitive);
		bool findStringAt( uint  row, uint  col, QString text, bool caseSensitive);

		bool findFirstRegExp( QString regexp);
		bool findNextRegExp( QString regexp);
		bool findPreviousRegExp( QString regexp);
		bool findLastRegExp( QString regexp);
		bool findRegExpAt( uint  row, uint  col, QString regexp);

		uint currentMatchLine();
		uint currentMatchCol();
		uint currentMatchLength();

	private:
		SearchInterface *m_parent;
		uint  m_currentcol;
		uint  m_currentrow;
		uint  m_currentmatchlen;
	};
};
#endif
