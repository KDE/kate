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

// $Id$

#include "katecodefoldinghelpers.h"
#include "katecodefoldinghelpers.moc"
#include <qstring.h>
#include <kdebug.h>
	

#define JW_DEBUG 0


KateCodeFoldingNode::KateCodeFoldingNode():parent(0),
		childnodes(0),startLineValid(false),startLineRel(0),endLineValid(false),
		endLineRel(0),type(0),visible(true),deleteOpening(false),deleteEnding(false){;}

KateCodeFoldingNode::KateCodeFoldingNode(KateCodeFoldingNode *par, signed char typ, unsigned int sLRel): parent(par),childnodes(0),
		startLineValid(true),startLineRel(sLRel),endLineValid(false),endLineRel(20000),type(typ),visible(true),
		deleteOpening(false),deleteEnding(false){;} //the endline fields should be initialised to not valid

KateCodeFoldingNode::~KateCodeFoldingNode(){;}

KateCodeFoldingTree::KateCodeFoldingTree(QObject *par):QObject(par),KateCodeFoldingNode()
{
	dontIgnoreUnchangedLines.setAutoDelete(true);
	lineMapping.setAutoDelete(true);
	hiddenLinesCountCacheValid=false;
	// initialize the root "special" node
	startLineRel=0;
	startLineValid=true; 
	endLineValid=true; // temporary, should be false;
	endLineRel=60000;   // temporary;
	childnodes=new QPtrList<KateCodeFoldingNode>; // since the root node has most probably child nodes allocate it
}

KateCodeFoldingTree::~KateCodeFoldingTree(){;}


bool KateCodeFoldingTree::isTopLevel(unsigned int line)
{
	for (unsigned int i=0;i<childnodes->count();i++)  // look if a given lines belongs to a sub node
	{
		KateCodeFoldingNode* node;
		node=childnodes->at(i);
		if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel))
		return false; // the line is within the range of a subnode -> return toplevel=false
	}
	return true; // the root node is the only node containing the given line, return toplevel=true
}

void KateCodeFoldingTree::getLineInfo(KateLineInfo *info,unsigned int line)
{
	//initialze the returned structure, this will also be returned if the root node has no child nodes
	// or the line is not within a childnode's range.
	info->topLevel=true;
	info->startsVisibleBlock=false;
	info->startsInVisibleBlock=false;
	info->endsBlock=false;
	info->invalidBlockEnd=false;

	//let's look for some information
        for (unsigned int i=0;i<childnodes->count();i++) // iterate over all root child nodes
        {
                KateCodeFoldingNode *node;
                node=childnodes->at(i);
                if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel)) // we found a node, which contains the given line -> do a complete lookup
		{
			info->topLevel=false;  //we are definitly not toplevel
			findAllNodesOpenedOrClosedAt(line);  //lookup all nodes, which start or and at the given line
			for (int i=0;i<nodesForLine.count();i++)
			{
				node=nodesForLine.at(i);
				unsigned int startLine=getStartLine(node);

				if (node->type<0)
				{
					info->invalidBlockEnd=true;	// type<0 means, that a region has been closed, but not opened
									// eg. parantheses missmatch
				}
				else
                                if (startLine != line)			// does the region we look at not start at the given line
                                info->endsBlock=true;			// than it has to be an ending
                                else
                                {
					// the line starts a new region, now determine, if it's a visible or a hidden region
                                        if (node->visible) info->startsVisibleBlock=true;
                                        else info->startsInVisibleBlock=true;
                                }
			}
			return;
		}
        }
        return ;
}


KateCodeFoldingNode *KateCodeFoldingTree::findNodeForLine(unsigned int line)
{
	// lets look, if given line is within a subnode range, and then return the deepest one.
	for (KateCodeFoldingNode *node=childnodes->first();node;node=childnodes->next())
	{
		if ((node->startLineRel<=line) && (line<=node->startLineRel+node->endLineRel))
			{
				// a region surounds the line, look in the next deeper hierarchy step
				return findNodeForLineDescending(node,line,0);
			}
	}
#if JW_DEBUG
	kdDebug()<<"Returning tree root"<<endl;
#endif
	return this; // the line is only contained by the root node
}

KateCodeFoldingNode *KateCodeFoldingTree::findNodeForLineDescending(KateCodeFoldingNode *node,
		 unsigned int line, unsigned int offset, bool oneStepOnly)
{
	offset=offset+node->startLineRel; // calculate the offset, between a subnodes real start line and its relative start
	if (node->childnodes)
	{
		for (KateCodeFoldingNode *subNode =node->childnodes->first();subNode;subNode=node->childnodes->next())
		{
			if ((subNode->startLineRel+offset<=line) && (line<=subNode->endLineRel+subNode->startLineRel+offset)) //warning fix me for invalid ends
			{
				// a subnode contains the line.
				// if oneStepOnly is true, we don't want to search for the deepest node, just return the found one
				if (oneStepOnly) return subNode;
				else return findNodeForLineDescending(subNode,line,offset); // look into the next deeper hierarchy step
			}
		}
	}

	return node; // the current node has no sub nodes, or the line couldn'te be found within a subregion
}


void KateCodeFoldingTree::debugDump()
{
	//dump all nodes for debugging
	kdDebug()<<"The parsed region/block tree for code folding"<<endl;
	dumpNode(this,"");
}

void KateCodeFoldingTree::dumpNode(KateCodeFoldingNode *node,QString prefix)
{
	//output node properties
	kdDebug()<<prefix<<QString("Type: %1, startLineValid %2, startLineRel %3, endLineValid %4, endLineRel %5").
			arg(node->type).arg(node->startLineValid).arg(node->startLineRel).arg(node->endLineValid).
			arg(node->endLineRel)<<endl;
	if (node->childnodes)
	{
		//output child node properties recursive
		prefix=prefix+"   ";
		for (unsigned int i=0;i<node->childnodes->count();i++)
			dumpNode(node->childnodes->at(i),prefix);
	}
	
}

/*
 That's one of the most important functions ;)
*/

void KateCodeFoldingTree::updateLine(unsigned int line,
	QMemArray<signed char> *regionChanges, bool *updated,bool changed)
{

	if (!changed)
	{
		if (dontIgnoreUnchangedLines.isEmpty()) return;
		if (dontIgnoreUnchangedLines[line])
		{
			dontIgnoreUnchangedLines.remove(line);
		} else return;
	}
	
//	kdDebug()<<QString("KateCodeFoldingTree:UpdateLine(): line %1").arg(line)<<endl;

	unsigned char tmp;

	something_changed=false;
#if JW_DEBUG
	kdDebug()<<QString("KateCodeFoldingTree::updateLine: Line %1").arg(line)<<endl;

	QString debugstr="KateCodeFoldingTree::updateLine:  Got list:";
	for (int i=regionChanges->size()-1;i>=0;i--)
	{
		debugstr=debugstr+QString("%1, ").arg((*regionChanges)[i]);
	}
	kdDebug()<<debugstr<<endl;
#endif
	for (int i=regionChanges->size()-1;i>0;i--)
	{
#if JW_DEBUG
		kdDebug()<<QString("i: %1, i-1: %2").arg((*regionChanges)[i]).arg((*regionChanges)[i-1])<<endl;
#endif
		if ((((*regionChanges)[i])>0) && (((*regionChanges)[i])==-((*regionChanges)[i-1])))
		{
    for (uint z4=i-1; (z4+2) < regionChanges->size(); z4++)
        (*regionChanges)[z4] = (*regionChanges)[z4+2];

      regionChanges->resize (regionChanges->size()-2);
		}
	}

#if JW_DEBUG
	kdDebug()<<QString("KateCodeFoldingTree::updateLine: Line %1").arg(line)<<endl;

	debugstr="KateCodeFoldingTree::updateLine:  Simplified list:";
	for (int i=regionChanges->size()-1;i>=0;i--)
	{
		debugstr=debugstr+QString("%1, ").arg((*regionChanges)[i]);
	}
	kdDebug()<<debugstr<<endl;
#endif

        findAndMarkAllNodesforRemovalOpenedOrClosedAt(line);

	if (regionChanges->isEmpty())
	{
//		KateCodeFoldingNode *node=findNodeForLine(line);
//		if (node->type!=0)
//		if (getStartLine(node)+node->endLineRel==line) removeEnding(node,line);
	}
	else
	{
		signed char data= (*regionChanges)[regionChanges->size()-1];
		regionChanges->resize (regionChanges->size()-1);

{
			int insertPos=-1;
			KateCodeFoldingNode *node;
			if (data<0)
			{
				node=findNodeForLine(line);
//				if (insertPos==-1)
				{
					if (node->childnodes)
					{
						unsigned int tmpLine=line-getStartLine(node);
						for (int i=0;i<node->childnodes->count();i++)
						{
							if (node->childnodes->at(i)->startLineRel>=tmpLine)
							{
								insertPos=i;
								break;
							}
						}
					}
				}
			}
			else
			{
				node=findNodeForLine(line);
				for (;(node->parent) && (getStartLine(node->parent)==line) && (node->parent->type!=0); node=node->parent);
				if ((getStartLine(node)==line) && (node->type!=0))
				{
					insertPos=node->parent->childnodes->find(node);
					node=node->parent;
				}
				else
				{
					if (node->childnodes)
					{
						for (int i=0;i<node->childnodes->count();i++)
						{
							if (getStartLine(node->childnodes->at(i))>=line)
							{
								insertPos=i;
								break;
							}
						}
					}
				}
			}

			do
			{
				if (data<0)
				{
					if (correctEndings(data,node,line,insertPos))
					{
						insertPos=node->parent->childnodes->find(node)+1;
						node=node->parent;
					}
					else
					{
						if (insertPos!=-1) insertPos++;
					}
				}
				else
				{
					int startLine=getStartLine(node);
					if ((insertPos==-1) || (insertPos==node->childnodes->count()))
					{
				                KateCodeFoldingNode *newNode=new KateCodeFoldingNode (node,data,line-startLine);
						something_changed=true;
						if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();
						node->childnodes->append(newNode);
						addOpening(newNode,data,regionChanges,line);
						insertPos=node->childnodes->find(newNode)+1;
					}
					else
					if (node->childnodes->at(insertPos)->startLineRel==line-startLine)
					{
						addOpening(node->childnodes->at(insertPos),data,regionChanges,line);
						insertPos++;;
					}
					else
					{
//						kdDebug()<<"ADDING NODE "<<endl;
				                KateCodeFoldingNode *newNode=new KateCodeFoldingNode (node,data,line-startLine);
						something_changed=true;
						if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();
						node->childnodes->insert(insertPos,newNode);
						addOpening(newNode,data,regionChanges,line);
						insertPos++;
					}
				}



				if (regionChanges->isEmpty())
				{
					data=0;
				}
				else
				{
					data=(*regionChanges)[regionChanges->size()-1];
		      regionChanges->resize (regionChanges->size()-1);
				}
			} while (data!=0);

		 }			
	}
	cleanupUnneededNodes(line);
//	if (something_changed) emit regionBeginEndAddedRemoved(line);
	(*updated)=something_changed;	
}


void KateCodeFoldingTree::removeOpening(KateCodeFoldingNode *node,unsigned int line)
{
	if (node->type==0) return;
	int mypos=node->parent->childnodes->find(node);
	signed char t=node->type;	
	KateCodeFoldingNode *p=node->parent;

	//move childnodes up
	if (node->childnodes)
	{
		for(;node->childnodes->count()>0;)
		{
			KateCodeFoldingNode *n;
			p->childnodes->insert(mypos,n=node->childnodes->take(0));
			n->parent=p;
			n->startLineRel=n->startLineRel+node->startLineRel;
			mypos++;
		}
	}
	

	// remove the node
	mypos=node->parent->childnodes->find(node);
	bool endLineValid=node->endLineValid;
	int endLineRel=node->endLineRel;
	delete node->parent->childnodes->take(mypos);
	if ((t>0) && (endLineValid))
		correctEndings(-t, p,line+endLineRel/*+1*/,mypos); // why the hell did I add a +1 here ?

}

void KateCodeFoldingTree::removeEnding(KateCodeFoldingNode *node,unsigned int line)
{
	if (node->type==0) return;
	if (node->type<0)
	{
		delete node->parent->childnodes->take(node->parent->childnodes->find(node));
		return;
	}
	int mypos=node->parent->childnodes->find(node);
	int count=node->parent->childnodes->count();
	for (int i=mypos+1;i<count;i++)
	{
		if (node->parent->childnodes->at(i)->type==-node->type)
		{
			node->endLineValid=true;
			node->endLineRel=node->parent->childnodes->at(i)->startLineRel-node->startLineRel;
			delete node->parent->childnodes->take(i);
			count=i-mypos-1;
			if (count>0)
			{
				if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();
				KateCodeFoldingNode *tmp;
				for (int i=0;i<count;i++)
				{
					tmp=node->parent->childnodes->take(mypos+1);
					tmp->startLineRel=tmp->startLineRel-node->startLineRel;
					node->childnodes->append(tmp);
				}
			}
			return;
		}
	}
	
	if (node->parent->type==node->type)
	{
		KateCodeFoldingNode *tmp;
		for (int i=mypos+1;i<node->parent->childnodes->count();i++)
		{
			tmp=node->parent->childnodes->take(mypos+1);
			tmp->startLineRel=tmp->startLineRel-node->startLineRel;
			node->childnodes->append(tmp);
			
		}
		// this should fix the bug of wrongly closed nodes
		node->endLineValid=node->parent->endLineValid;
		node->endLineRel=node->parent->endLineRel-node->startLineRel;
		if (node->endLineValid) removeEnding(node->parent,getStartLine(node->parent)+node->parent->endLineRel);
		return;
	}

	node->endLineValid=false;
	node->endLineRel=node->parent->endLineRel-node->startLineRel;
	
}


bool KateCodeFoldingTree::correctEndings(signed char data, KateCodeFoldingNode *node,unsigned int line,int insertPos)
{
	unsigned int startLine=getStartLine(node);
	if (data!=-node->type)
	{
#if JW_DEBUG
		kdDebug()<<"data!=-node->type (correctEndings)"<<endl;
#endif
		//invalid close -> add to unopend list
		dontDeleteEnding(node);
		if (data==node->type) return false;
		KateCodeFoldingNode *newNode=new KateCodeFoldingNode (node,data,line-startLine);
		something_changed=true;
		newNode->startLineValid=false;
		newNode->endLineValid=true;
		newNode->endLineRel=0;
		if (node->childnodes)
		{
			if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();
			if ((insertPos==-1) || (insertPos==node->childnodes->count()))
			{
				node->childnodes->append(newNode);
			}
			else
			{
				node->childnodes->insert(insertPos,newNode);
			}
			// find correct position
		}
		else
		{
			node->childnodes=new QPtrList<KateCodeFoldingNode>();
			node->childnodes->append(newNode);
		}
		return false;
	}
	else
	{
		dontDeleteEnding(node);	

		// valid closing region
		unsigned int startLine=getStartLine(node);
		if (!node->endLineValid)
		{
			node->endLineValid=true;
			node->endLineRel=line-startLine;
			//moving
		}
		else
		{
#if JW_DEBUG
			kdDebug()<<"Closing a node which had already a valid end"<<endl;
#endif
			// block has already an ending
			if (startLine+node->endLineRel==line)
			{
				 // we won, just skip
#if JW_DEBUG
				kdDebug()<< "We won, just skipping (correctEndings)"<<endl;
#endif
			}
			else
			{
				int bakEndLine=node->endLineRel+startLine;
				node->endLineRel=line-startLine;

				int mypos=node->parent->childnodes->find(node);

				if (node->childnodes)
				{
#if JW_DEBUG
					kdDebug()<< "reclosed node had childnodes"<<endl;
#endif
#if JW_DEBUG
					kdDebug()<<"It could be, that childnodes need to be moved up"<<endl;
#endif
					int removepos=-1;
					int cnt=node->childnodes->count();
					for (int i=0;i<cnt;i++)
						if (node->childnodes->at(i)->startLineRel>=node->endLineRel)
						{
							removepos=i;
							break;
						}
#if JW_DEBUG
					kdDebug()<<QString("remove pos: %1").arg(removepos)<<endl;
#endif
					if (removepos>-1)
					{
#if JW_DEBUG
						kdDebug()<<"Children need to be moved"<<endl;
#endif
						KateCodeFoldingNode *moveNode;
						if (mypos==node->parent->childnodes->count()-1)
						{
							while (removepos<node->childnodes->count())
							{
								node->parent->childnodes->append(moveNode=node->childnodes->take(removepos));
								moveNode->startLineRel +=node->startLineRel;
							}
						}
						else
						{
							int insertPos=mypos;
							while (removepos<node->childnodes->count())
								{
									insertPos++;
									node->parent->childnodes->insert(insertPos,moveNode=node->childnodes->take(removepos));
									moveNode->startLineRel +=node->startLineRel;
								}
						}
					}
												
				}

				if (node->parent)
				{

					int idToClose=-1;

#if 0 //this can never happen anymore
					for (int i=mypos-1;i>=0;i--)
						if (node->parent->childnodes->at(i)->type==node->type)
						{
							if (node->parent->childnodes->current()->endLineValid==false)
							{
								idToClose=i;
								correctEndings(data,node->parent->childnodes->current(),bakEndLine,-1); // -1 ??
							}
						}
#endif
					if (idToClose==-1)
					{
#if JW_DEBUG
						kdDebug()<<"idToClose == -1 --> close parent"<<endl;
#endif
						correctEndings(data,node->parent,bakEndLine, node->parent->childnodes->find(node)+1); // ????
					}
				}
				else
				{
					//add to unopened list (bakEndLine)
				}
			}
		}
		return true;
	}
}






void KateCodeFoldingTree::addOpening(KateCodeFoldingNode *node,signed char nType, QMemArray<signed char>* list,unsigned int line)
{
	unsigned int startLine=getStartLine(node);
	if ((startLine==line) && (node->type!=0))
	{
#if JW_DEBUG
		kdDebug()<<"startLine equals line"<<endl;
#endif
		if (nType==node->type)
		{
#if JW_DEBUG
			kdDebug()<<"Node exists"<<endl;
#endif
			node->deleteOpening=false;

			if (!node->endLineValid)
			{
				int current=node->parent->childnodes->find(node);
				int count=node->parent->childnodes->count()-(current+1);
				node->endLineRel=node->parent->endLineRel-node->startLineRel;

// EXPERIMENTAL TEST BEGIN
// move this afte the test for unopened, but closed regions within the parent node, or if there are no siblings, bubble up
			if (node->parent)
			if (node->parent->type==node->type)
			{
				if (node->parent->endLineValid)
				{
					removeEnding(node->parent,line);
					node->endLineValid=true;
				}
			}

// EXPERIMENTAL TEST BEGIN

				if (current!=node->parent->childnodes->count()-1)
				{
					if (node->type!=node->parent->type) //FIX ME, it should be searched for an unopened but closed region, even if the parent is of the same type
					{
						for (int i=current+1;i<node->parent->childnodes->count();i++)
						{
							if (node->parent->childnodes->at(i)->type==-node->type)
							{
								count=(i-current-1);
								node->endLineValid=true;
								node->endLineRel=getStartLine(node->parent->childnodes->at(i))-line;
								delete node->parent->childnodes->take(i);
								break;
							}
						}
					}
					else
					{
						node->parent->endLineValid=false;
						node->parent->endLineRel=20000;
					}
					if (count>0)
					{
						if (!node->childnodes)	node->childnodes=new QPtrList<KateCodeFoldingNode>();
						for (int i=0;i<count;i++)
						{
							KateCodeFoldingNode *n;
							node->childnodes->append(n=node->parent->childnodes->take(current+1));
							n->startLineRel=n->startLineRel-node->startLineRel;
							n->parent=node;
//							kdDebug()<<"Moving node UP !!!!!!!!!!"<<endl;
						}
					}
				}

			}

			addOpening_further_iterations(node,nType,list,line,0,startLine);

		} //else ohoh, much work to do same line, but other region type
	}
	else
	{ // create a new region
		KateCodeFoldingNode *newNode=new KateCodeFoldingNode (node,nType,line-startLine);
		something_changed=true;
		if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();


		int insert_position=-1;
		if (node->childnodes->count()>0)
		{
			for (int i=0;i<node->childnodes->count();i++)
			{
				if (startLine+node->childnodes->at(i)->startLineRel>line)
				{
					 insert_position=i;
					 break;
				}
			}
		}
		
		int current;
		if (insert_position==-1)
		{
			node->childnodes->append(newNode);
			current=node->childnodes->count()-1;
		}
		else
		{
			node->childnodes->insert(insert_position,newNode);
			current=insert_position;
		}
		
//		if (node->type==newNode->type)
//		{
//			newNode->endLineValid=true;
//			node->endLineValid=false;
//			newNode->endLineRel=node->endLineRel-newNode->startLineRel;
//			node->endLineRel=20000; //FIXME
			int count=node->childnodes->count()-(current+1);
			newNode->endLineRel=node->endLineRel-newNode->startLineRel;
			if (current!=node->childnodes->count()-1)
			{
				if (node->type!=newNode->type)
				{
					for (int i=current+1;i<node->childnodes->count();i++)
					{
						if (node->childnodes->at(i)->type==-newNode->type)
						{
							count=node->childnodes->count()-i-1;
							newNode->endLineValid=true;
							newNode->endLineRel=line-getStartLine(node->childnodes->at(i));
							delete node->childnodes->take(i);
							break;
						}
					}
				}
				else
				{
					node->endLineValid=false;
					node->endLineRel=20000;
				}
				if (count>0)
				{
					newNode->childnodes=new QPtrList<KateCodeFoldingNode>();
					for (int i=0;i<count;i++)
					{
						newNode->childnodes->append(node->childnodes->take(current+1));
					}
				}
//			}
		}
		
		addOpening(newNode,nType,list,line);

		addOpening_further_iterations(node,node->type,list,line,current,startLine);
	
	}
}





void KateCodeFoldingTree::addOpening_further_iterations(KateCodeFoldingNode *node,signed char nType, QMemArray<signed char>*
		list,unsigned int line,int current, unsigned int startLine)
{
	while (!(list->isEmpty()))
	{
		if (list->isEmpty()) return;
		else
		{
       signed char data=(*list)[list->size()-1];
		   list->resize (list->size()-1);

      if (data<0)
			{
#if JW_DEBUG
				kdDebug()<<"An ending was found"<<endl;
#endif


				if (correctEndings(data,node,line,-1)) return; // -1 ?

#if 0
				if(data == -nType)
				{
					if (node->endLineValid)
					{
						if (node->endLineRel+startLine==line) // We've won again
						{
							//handle next node;
						}
						else
						{ // much moving
							node->endLineRel=line-startLine;
							node->endLineValid=true;
						}
						return;	// next higher level should do the rest
					}
					else
					{
						node->endLineRel=line-startLine;
						node->endLineValid=true;
						//much moving
					}
				} //else add to unopened list
#endif
			}
			else
			{
				bool needNew=true;
				if (node->childnodes)
				{
					if (current<node->childnodes->count())
					{
						if (getStartLine(node->childnodes->at(current)) == line)
							needNew=false;
					}
				}
				if (needNew)
				{
					something_changed=true;
					KateCodeFoldingNode *newNode=new KateCodeFoldingNode(node,data,
						line-startLine);
					if (!node->childnodes) node->childnodes=new QPtrList<KateCodeFoldingNode>();
					node->childnodes->insert(current,newNode);	//find the correct position later
				 }
             					addOpening(node->childnodes->at(current),data,list,line);
					current++;
				//lookup node or create subnode
			}
		}
	} // end while
}

unsigned int KateCodeFoldingTree::getStartLine(KateCodeFoldingNode *node)
{
	unsigned int lineStart=0;
	for (KateCodeFoldingNode *node1=node;node1->type!=0;node1=node1->parent) lineStart=lineStart+node1->startLineRel;
	return lineStart;
}












void KateCodeFoldingTree::lineHasBeenRemoved(unsigned int line)
{
	lineMapping.clear();
	dontIgnoreUnchangedLines.insert(line,new bool(true));
	dontIgnoreUnchangedLines.insert(line-1,new bool(true));
	dontIgnoreUnchangedLines.insert(line+1,new bool(true));

	hiddenLinesCountCacheValid=false;
//#if JW_DEBUG
//	kdDebug()<<QString("KateCodeFoldingTree::lineHasBeenRemoved: %1").arg(line)<<endl;
//#endif

//line ++;
	findAndMarkAllNodesforRemovalOpenedOrClosedAt(line); //It's an ugly solution
	cleanupUnneededNodes(line);	//It's an ugly solution

	KateCodeFoldingNode *node=findNodeForLine(line);
//?????	if (node->endLineValid)
	{
		int startLine=getStartLine(node);
		if (startLine==line) node->startLineRel--; else
		{
			if (node->endLineRel==0) node->endLineValid=false;
			node->endLineRel--;
		}
		if (node->childnodes)
		{
			int cnt=node->childnodes->count();
			for (int i=0;i<cnt;i++)
			{
				if (node->childnodes->at(i)->startLineRel+startLine>=line)
				{
						node->childnodes->at(i)->startLineRel--;
				}
			}
		}
	}

	if (node->parent)
	decrementBy1(node->parent,node);
}


void KateCodeFoldingTree::decrementBy1(KateCodeFoldingNode *node, KateCodeFoldingNode *after)
{
	if (node->endLineRel==0) node->endLineValid=false;
	node->endLineRel--;

	node->childnodes->find(after);
	KateCodeFoldingNode *tmp;
	while ((tmp=node->childnodes->next()))
	{
		tmp->startLineRel--;
	}

	if (node->parent)
	{
		decrementBy1(node->parent,node);
	}
}


void KateCodeFoldingTree::lineHasBeenInserted(unsigned int line)
{
	lineMapping.clear();
        dontIgnoreUnchangedLines.insert(line,new bool(true));
        dontIgnoreUnchangedLines.insert(line-1,new bool(true));
        dontIgnoreUnchangedLines.insert(line+1,new bool(true));
	hiddenLinesCountCacheValid=false;
//return;
#if JW_DEBUG
	kdDebug()<<QString("KateCodeFoldingTree::lineHasBeenInserted: %1").arg(line)<<endl;
#endif 

//	findAndMarkAllNodesforRemovalOpenedOrClosedAt(line);
//	cleanupUnneededNodes(line);

	KateCodeFoldingNode *node=findNodeForLine(line);
// ????????	if (node->endLineValid)
	{
		int startLine=getStartLine(node);
		if (node->type<0) node->startLineRel++; else node->endLineRel++;
		if (node->childnodes)
		{

			int cnt=node->childnodes->count();
			for (KateCodeFoldingNode *tmpNode=node->childnodes->first();tmpNode;tmpNode=node->childnodes->next())
			{
				if (tmpNode->startLineRel+startLine>=line)
				{
					tmpNode->startLineRel++;
				}
			}
		}
	}

	if (node->parent)
	incrementBy1(node->parent,node);

        for (QValueList<hiddenLineBlock>::Iterator it=hiddenLines.begin(); it!=hiddenLines.end();++it)
	{
		if ((*it).start>line)
		{
			(*it).start++;
		}
		else
		if ((*it).start+(*it).length>line)
		{
			(*it).length++;
		}
	}

}

void KateCodeFoldingTree::incrementBy1(KateCodeFoldingNode *node, KateCodeFoldingNode *after)
{
	node->endLineRel++;

	node->childnodes->find(after);
	KateCodeFoldingNode *tmp;
	while ((tmp=node->childnodes->next()))
	{
		tmp->startLineRel++;
	}

	if (node->parent)
	{
		incrementBy1(node->parent,node);
	}
}



void KateCodeFoldingTree::findAndMarkAllNodesforRemovalOpenedOrClosedAt(unsigned int line)
{

#warning "FIXME:  make this multiple region changes per line save";
//	return;
	markedForDeleting.clear();
	KateCodeFoldingNode *node=findNodeForLine(line);
	if (node->type==0) return;
	unsigned int startLine=getStartLine(node);
	if ((startLine==line) && (node->startLineValid))  node->deleteOpening=true;
	if ((startLine+node->endLineRel==line) || ((node->endLineValid==false) &&  (node->deleteOpening))) node->deleteEnding=true;
	markedForDeleting.append(node);
	while (((node->parent) && (node->parent->type!=0)) && (getStartLine(node->parent)==line))
	{
		node=node->parent;
		addNodeToRemoveList(node,line);
	}
#if JW_DEBUG
	kdDebug()<<" added line to markedForDeleting list"<<endl;
#endif
}


void KateCodeFoldingTree::addNodeToRemoveList(KateCodeFoldingNode *node,unsigned int line)
{
#warning "FIXME:  make this multiple region changes per line save";
	unsigned int startLine=getStartLine(node);
	if ((startLine==line) && (node->startLineValid)) node->deleteOpening=true;
	if ((startLine+node->endLineRel==line) || ((node->endLineValid==false) &&  (node->deleteOpening))) node->deleteEnding=true;
	markedForDeleting.append(node);

}






void KateCodeFoldingTree::findAllNodesOpenedOrClosedAt(unsigned int line)
{
	nodesForLine.clear();
	KateCodeFoldingNode *node=findNodeForLine(line);
	if (node->type==0) return;
	unsigned int startLine=getStartLine(node);
	if (startLine==line) nodesForLine.append(node);
	else
	if ((startLine+node->endLineRel==line)) nodesForLine.append(node);
	while ((node->parent))
	{
		addNodeToFoundList(node->parent,line,node->parent->childnodes->find(node));
		node=node->parent;
	}
#if JW_DEBUG
	kdDebug()<<" added line to markedForDeleting list"<<endl;
#endif
}


void KateCodeFoldingTree::addNodeToFoundList(KateCodeFoldingNode *node,unsigned int line,int childpos)
{
	unsigned int startLine=getStartLine(node);
	if ((startLine==line) && (node->type!=0)) nodesForLine.append(node);
	else
	if ((startLine+node->endLineRel==line) && (node->type!=0)) nodesForLine.append(node);
	
	for (int i=childpos+1;i<node->childnodes->count();i++)
	{
		KateCodeFoldingNode *child=node->childnodes->at(i);
		if (startLine+child->startLineRel==line)
		{
			nodesForLine.append(child);
			if (child->childnodes)
			addNodeToFoundList(child,line,0);
		}
		else
			break;
	}

}















void KateCodeFoldingTree::cleanupUnneededNodes(unsigned int line)
{

#if JW_DEBUG
	kdDebug()<<"void KateCodeFoldingTree::cleanupUnneededNodes(unsigned int line)"<<endl;
#endif

//	return;
	if (markedForDeleting.isEmpty()) return;
	
	for (int i=0;i<markedForDeleting.count();i++)
	{
		KateCodeFoldingNode *n=markedForDeleting.at(i);
		if ((n->deleteOpening)) kdDebug()<<"DELETE OPENING SET"<<endl;
		if ((n->deleteEnding)) kdDebug()<<"DELETE ENDING SET"<<endl;

		if ((n->deleteOpening) && (n->deleteEnding))
		{
#if JW_DEBUG
			kdDebug()<<"Deleting complete node"<<endl;
#endif
			if (n->endLineValid)		// just delete it, it has been opened and closed on this line
			{
				delete n->parent->childnodes->take(n->parent->childnodes->find(n));
				something_changed=true;
			}
			else
			{
				removeOpening(n,line);
				dontDeleteOpening(n);
				dontDeleteEnding(n);
				// the node has subnodes which need to be moved up and this one has to be deleted
			}
			something_changed=true;
		}
		else
		if ((n->deleteOpening) && (n->startLineValid))
		{
#if JW_DEBUG
			kdDebug()<<"calling removeOpening"<<endl;
#endif
			if (n->type>0)
			removeOpening(n,line);
			something_changed=true;
			dontDeleteOpening(n);

		}
		else
		{
			dontDeleteOpening(n);
			if ((n->deleteEnding) && (n->endLineValid))
			{
//#if JW_DEBUG
				kdDebug()<<"calling removeEnding"<<endl;
//#endif
				removeEnding(n,line);
				something_changed=true;
				dontDeleteEnding(n);
			}
			else
				dontDeleteEnding(n);
		}
	}
//	debugDump();
}

void KateCodeFoldingTree::dontDeleteEnding(KateCodeFoldingNode* node)
{
	node->deleteEnding=false;
}


void KateCodeFoldingTree::dontDeleteOpening(KateCodeFoldingNode* node)
{
	node->deleteOpening=false;
}


void KateCodeFoldingTree::toggleRegionVisibility(unsigned int line)
{
	lineMapping.clear();
	hiddenLinesCountCacheValid=false;
//	kdDebug()<<QString("KateCodeFoldingTree::toggleRegionVisibility() %1").arg(line)<<endl;
	findAllNodesOpenedOrClosedAt(line);
	for (int i=0;i<nodesForLine.count();i++)
	{
		if (getStartLine(nodesForLine.at(i)) != line)
		{
			nodesForLine.remove(i);
			i--;
		}
	}
	if (nodesForLine.isEmpty()) return;
	nodesForLine.at(0)->visible=!nodesForLine.at(0)->visible;
	// just for testing, no nested regions are handled yet and not optimized at all
#if 0
	for (unsigned int i=line+1;i<=nodesForLine.at(0)->endLineRel+line;i++)
	{
//		kdDebug()<<QString("emit setLineVisible(%1,%2)").arg(i).arg(nodesForLine.at(0)->visible)<<endl;
		emit(setLineVisible(i,nodesForLine.at(0)->visible));
	}
#endif
	if (!nodesForLine.at(0)->visible)
	{
		addHiddenLineBlock(nodesForLine.at(0),line);

	}
	else
	{
	        for (QValueList<hiddenLineBlock>::Iterator it=hiddenLines.begin(); it!=hiddenLines.end();++it)
			if ((*it).start==line+1)
			{
				hiddenLines.remove(it);
				break;
			}
		for (unsigned int i=line+1;i<=nodesForLine.at(0)->endLineRel+line;i++)
		{
			emit(setLineVisible(i,true));
		}

		updateHiddenSubNodes(nodesForLine.at(0));
		
	}
	emit regionVisibilityChangedAt(line);
}

void KateCodeFoldingTree::updateHiddenSubNodes(KateCodeFoldingNode *node)
{
	if (!node->childnodes) return;
	for (KateCodeFoldingNode *n1=node->childnodes->first();n1;n1=node->childnodes->next())
	{
		if (!n1->visible)
		{
			addHiddenLineBlock(n1,getStartLine(n1));
		}
		else
		updateHiddenSubNodes(n1);
	}
	return;
}

void KateCodeFoldingTree::addHiddenLineBlock(KateCodeFoldingNode *node,unsigned int line)
{
		struct hiddenLineBlock data;
		data.start=line+1;
	        data.length=node->endLineRel;
		bool inserted=false;

	        for (QValueList<hiddenLineBlock>::Iterator it=hiddenLines.begin(); it!=hiddenLines.end();++it)
		{
			if (((*it).start>=data.start) && ((*it).start<=data.start+data.length-1)) // another hidden block starting at the within this block already exits -> adapt new block
			{
				// the existing block can't have lines behind the new one, because a newly hidden 
				//	block has to encapsulate already hidden ones
				it=hiddenLines.remove(it);
				--it;
/*				if (it==hiddenLines.end())
				{
					hiddenLines.insert(it,data);
					inserted=true;
				}
				break;*/
			}
			else
			if ((*it).start>line)
			{
				hiddenLines.insert(it,data);
				inserted=true;
				break;
			}
		}
		if (!inserted) hiddenLines.append(data);

                for (unsigned int i=line+1;i<=node->endLineRel+line;i++)
                {
                        emit(setLineVisible(i,false));
                }
}





unsigned int KateCodeFoldingTree::getRealLine(unsigned int virtualLine)
{
//	kdDebug()<<QString("VirtualLine %1").arg(virtualLine)<<endl;
	unsigned int *real=lineMapping[virtualLine];
	if (real) return (*real);
	unsigned int tmp=virtualLine;
	for (QValueList<hiddenLineBlock>::ConstIterator it=hiddenLines.begin();it!=hiddenLines.end();++it)
	{
		if ((*it).start<=virtualLine) virtualLine+=(*it).length;
		else break;
	}
//	kdDebug()<<QString("Real Line %1").arg(virtualLine)<<endl;
	lineMapping.insert(tmp,new unsigned int(virtualLine));
	return virtualLine;
}

unsigned int KateCodeFoldingTree::getVirtualLine(unsigned int realLine)
{
//	kdDebug()<<QString("RealLine--> %1").arg(realLine)<<endl;
	for (QValueList<hiddenLineBlock>::ConstIterator it=hiddenLines.fromLast();it!=hiddenLines.end();--it)
	{
		if ((*it).start<=realLine) realLine-=(*it).length;
		else break;
	}
//	kdDebug()<<QString("-->virtual Line %1").arg(realLine)<<endl;
	return realLine;
}


unsigned int KateCodeFoldingTree::getHiddenLinesCount()
{
	if (hiddenLinesCountCacheValid) return hiddenLinesCountCache;
	unsigned int tmp=0;
	for (QValueList<hiddenLineBlock>::ConstIterator it=hiddenLines.begin();
		it!=hiddenLines.end(); ++it) tmp=tmp+(*it).length;
	hiddenLinesCountCache=tmp;
	hiddenLinesCountCacheValid=true;
	return tmp;

}
