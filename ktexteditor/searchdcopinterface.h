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
		bool findStringAt( unsigned int row, unsigned int col, QString text, bool caseSensitive);

		bool findFirstRegExp( QString regexp);
		bool findNextRegExp( QString regexp);
		bool findPreviousRegExp( QString regexp);
		bool findLastRegExp( QString regexp);
		bool findRegExpAt( unsigned int row, unsigned int col, QString regexp);

		unsigned int currentMatchLine();
		unsigned int currentMatchCol();
		unsigned int currentMatchLength();

	private:
		SearchInterface *m_parent;
		unsigned int m_currentcol;
		unsigned int m_currentrow;
		unsigned int m_currentmatchlen;
	};
};
#endif
