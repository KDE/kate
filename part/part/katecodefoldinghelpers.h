/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_CODEFOLDING_HELPERS_
#define _KATE_CODEFOLDING_HELPERS_

//BEGIN INCLUDES + FORWARDS
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qobject.h>
#include <qintdict.h>

class KateCodeFoldingTree;
class QString;
//END

struct hiddenLineBlock
{
	unsigned int start;
	unsigned int length;
};

class KateLineInfo
{
public:
	bool topLevel;
	bool startsVisibleBlock;
	bool startsInVisibleBlock;
	bool endsBlock;
	bool invalidBlockEnd;
};

class KateCodeFoldingNode
{
public:
	
	KateCodeFoldingNode();
	KateCodeFoldingNode(KateCodeFoldingNode *par, signed char typ, unsigned int sLRel);
	~KateCodeFoldingNode();

protected:
	friend class KateCodeFoldingTree;

	KateCodeFoldingNode				*parentNode;
	QPtrList<KateCodeFoldingNode>	*childnodes;

	unsigned int startLineRel;	
	unsigned int endLineRel;

	bool startLineValid;
	bool endLineValid;

	signed char type;				// 0 -> toplevel / invalid
	bool visible;
	bool deleteOpening;
	bool deleteEnding;
};


class KateCodeFoldingTree : public QObject, public KateCodeFoldingNode
{
Q_OBJECT
public:
	KateCodeFoldingTree (QObject *);
	~KateCodeFoldingTree ();

	KateCodeFoldingNode *findNodeForLine (unsigned int line);

	unsigned int getRealLine	(unsigned int virtualLine);
	unsigned int getVirtualLine	(unsigned int realLine);
	unsigned int getHiddenLinesCount ();

	bool isTopLevel (unsigned int line);

//	void updateLine(unsigned int line,QValueList<signed char> *regionChanges);
	void lineHasBeenInserted(unsigned int line);
	void lineHasBeenRemoved	(unsigned int line);
	void debugDump();
	void getLineInfo(KateLineInfo *info,unsigned int line);
private:
	QIntDict<unsigned int>	lineMapping;
	QIntDict<bool>			dontIgnoreUnchangedLines;

	QPtrList<KateCodeFoldingNode>	markedForDeleting;
	QPtrList<KateCodeFoldingNode>	nodesForLine;
	QValueList<hiddenLineBlock>		hiddenLines;

	unsigned int	hiddenLinesCountCache;
	bool			something_changed;
	bool			hiddenLinesCountCacheValid;

	KateCodeFoldingNode *findNodeForLineDescending(KateCodeFoldingNode *, unsigned int, unsigned int,bool oneStepOnly=false);

	unsigned int getStartLine(KateCodeFoldingNode *node);

	bool correctEndings	(signed char data, KateCodeFoldingNode *node, unsigned int line, int insertPos);

	void dumpNode	(KateCodeFoldingNode *node,QString prefix);
	void addOpening	(KateCodeFoldingNode *node, signed char nType,QMemArray<signed char>* list, unsigned int line);
	void addOpening_further_iterations (KateCodeFoldingNode *node,signed char nType, QMemArray<signed char>*
										list,unsigned int line,int current,unsigned int startLine);

	void incrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);
	void decrementBy1 (KateCodeFoldingNode *node, KateCodeFoldingNode *after);

	void cleanupUnneededNodes (unsigned int line);
	void removeEnding	(KateCodeFoldingNode *node,unsigned int line);
	void removeOpening	(KateCodeFoldingNode *node,unsigned int line);

	void findAndMarkAllNodesforRemovalOpenedOrClosedAt (unsigned int line);
	void findAllNodesOpenedOrClosedAt (unsigned int line);

	void addNodeToFoundList	(KateCodeFoldingNode *node,unsigned int line,int childpos);
	void addNodeToRemoveList(KateCodeFoldingNode *node,unsigned int line);
	void addHiddenLineBlock	(KateCodeFoldingNode *node,unsigned int line);

	void dontDeleteEnding	(KateCodeFoldingNode*);
	void dontDeleteOpening	(KateCodeFoldingNode*);

	void updateHiddenSubNodes (KateCodeFoldingNode *node);
public slots:

	void updateLine (unsigned int line,QMemArray<signed char>* regionChanges, bool *updated, bool changed);
	void toggleRegionVisibility (unsigned int);

signals:
	void setLineVisible (unsigned int, bool);
	void regionVisibilityChangedAt	(unsigned int);
	void regionBeginEndAddedRemoved	(unsigned int);
};

#endif
