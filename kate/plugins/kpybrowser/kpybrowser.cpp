/***************************************************************************
                          kpybrowser.cpp  -  description
                             -------------------
    begin                : Fri Aug 24 15:11:58 MST 2001
    copyright            : (C) 2001 by Christian Bird
    email                : chrisb@lineo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kpybrowser.h"
#include <q3header.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3ValueList>
#include <kmessagebox.h>
#include <klocale.h>
#include <qregexp.h>
#include <k3listview.h>
#include "kpybrowser.moc"

#include <iostream>

static const char *container_xpm[] = {
  "16 16 119 2",
  "       c None",
  ".      c #020202",
  "+      c #484848",
  "@      c #141414",
  "#      c #CBCBCB",
  "$      c #E9E9E9",
  "%      c #2F2F2F",
  "&      c #3E3E3E",
  "*      c #006B9E",
  "=      c #003D5A",
  "-      c #757575",
  ";      c #A4A4A4",
  ">      c #727272",
  ",      c #282828",
  "'      c #C1E1ED",
  ")      c #D3EDF6",
  "!      c #79BFD6",
  "~      c #E4E4E4",
  "{      c #202121",
  "]      c #3CB9ED",
  "^      c #3AA3C5",
  "/      c #77BCD6",
  "(      c #82C3D9",
  "_      c #0873A5",
  ":      c #003C5B",
  "<      c #464646",
  "[      c #1E1E1E",
  "}      c #19AFEE",
  "|      c #0787B6",
  "1      c #38A2C5",
  "2      c #5DB1D0",
  "3      c #76BBD5",
  "4      c #81C2D8",
  "5      c #85C4D9",
  "6      c #0770A1",
  "7      c #4F4F4F",
  "8      c #169ACB",
  "9      c #106CA1",
  "0      c #127CAD",
  "a      c #0784B2",
  "b      c #56B0CD",
  "c      c #72B9D4",
  "d      c #91C9DB",
  "e      c #3F3F3F",
  "f      c #2B2B2B",
  "g      c #1380B2",
  "h      c #0B5482",
  "i      c #0E5E91",
  "j      c #0E6597",
  "k      c #116FA0",
  "l      c #127FB1",
  "m      c #58AECD",
  "n      c #70B8D3",
  "o      c #454545",
  "p      c #FCFCFC",
  "q      c #E9E4E4",
  "r      c #8AA9B9",
  "s      c #296C8E",
  "t      c #0E5C8D",
  "u      c #0F6698",
  "v      c #1175A4",
  "w      c #404040",
  "x      c #444444",
  "y      c #F6F6F6",
  "z      c #F1F1F1",
  "A      c #EBEBEB",
  "B      c #EAEAEA",
  "C      c #DBD7D6",
  "D      c #81A0B0",
  "E      c #286C8F",
  "F      c #0F6596",
  "G      c #1172A4",
  "H      c #57A1C0",
  "I      c #424242",
  "J      c #EEEEEE",
  "K      c #E6E6E6",
  "L      c #E1E1E1",
  "M      c #DCDCDC",
  "N      c #D5D5D5",
  "O      c #D7D7D7",
  "P      c #CBC9C8",
  "Q      c #88A6B5",
  "R      c #276C8C",
  "S      c #276892",
  "T      c #363636",
  "U      c #C4C4C4",
  "V      c #DFDFDF",
  "W      c #DEDEDE",
  "X      c #DBDBDB",
  "Y      c #D6D6D6",
  "Z      c #D1D1D1",
  "`      c #CCCCCC",
  " .     c #C5C5C5",
  "..     c #BFBFBF",
  "+.     c #C1BEBE",
  "@.     c #6E6D6D",
  "#.     c #686868",
  "$.     c #C0C0C0",
  "%.     c #CECECE",
  "&.     c #CACACA",
  "*.     c #C6C6C6",
  "=.     c #BBBBBB",
  "-.     c #B6B6B6",
  ";.     c #AEAEAE",
  ">.     c #323232",
  ",.     c #606060",
  "'.     c #AFAFAF",
  ").     c #B5B5B5",
  "!.     c #B0B0B0",
  "~.     c #AAAAAA",
  "{.     c #6C6B6B",
  "].     c #2E2E2E",
  "^.     c #585858",
  "/.     c #9D9D9D",
  "(.     c #A7A7A7",
  "_.     c #6A6969",
  ":.     c #393939",
  "<.     c #292929",
  "[.     c #3A3835",
  "        . +                     ",
  "      @ # $ + + %               ",
  "    & * = - ; # $ + . + > .     ",
  "    , ' ) ! * = - ; # ~ $ $ .   ",
  "    { ] ^ / ' ) ( _ : - ; # $ < ",
  "    [ } | 1 2 3 4 ' ) 5 6 = - 7 ",
  "    [ 8 9 0 a | 1 b c d ' ) ! e ",
  "    f g h i j k l | | 1 m n ' e ",
  "    o p q r s h t u v 0 | | n w ",
  "    x y z A B C D E h i F G H w ",
  "    I J B K L M N O P Q R h S w ",
  "    T U V W X Y Z `  ... .+.@.o ",
  "      . T #.$.%.&.*.$.=.-.;.@.< ",
  "            . >.,.'.=.).!.~.{.o ",
  "                  . ].^./.(._.:.",
  "                        . <.[.  "
};

void
getOpenNodes (Q3ValueList < QString > *open_nodes, PyBrowseNode * node)
{
  if (node == NULL)
    return;
  if (node->isOpen ())
    {
      open_nodes->append (node->getQualifiedName ());
    }

  getOpenNodes (open_nodes,
		dynamic_cast < PyBrowseNode * >(node->firstChild ()));
  getOpenNodes (open_nodes,
		dynamic_cast < PyBrowseNode * >(node->nextSibling ()));
}


KPyBrowser::KPyBrowser (QWidget * parent, const char *name):
K3ListView (parent)
{

  addColumn(i18n("Name"));
  header ()->hide ();
  class_root =
    new PyBrowseNode (this, QString ("Classes"), i18n("Classes"),
		      PYOTHER);
  class_root->setPixmap (0, QPixmap (container_xpm));
  function_root =
    new PyBrowseNode (this, QString ("Globals"), i18n("Globals"),
		      PYOTHER);
  function_root->setPixmap (0, QPixmap (container_xpm));
  setRootIsDecorated (1);
  connect (this, SIGNAL (executed (Q3ListViewItem *)), this,
	   SLOT (nodeSelected (Q3ListViewItem *)));
  setTooltipColumn (1);
  setShowToolTips (1);
 // tooltip = new KPBToolTip (this);
}

KPyBrowser::~KPyBrowser ()
{
//  delete tooltip;
}

void
KPyBrowser::nodeSelected (Q3ListViewItem * node)
{
  PyBrowseNode *browse_node = dynamic_cast < PyBrowseNode * >(node);

  if (!browse_node)
    {
      return;
    }

  QString method_name;
  int line_no;

  line_no = browse_node->getLine();
  method_name = browse_node->getName();
  if (browse_node->getType () == PYCLASS)
    {
      method_name = QString ("class ") + browse_node->getName ();
    }
  else if (browse_node->getType () == PYMETHOD
	   || browse_node->getType () == PYFUNCTION)
    {
      method_name = QString ("def ") + browse_node->getName ();
    }
  emit selected(method_name, line_no);
}

void
KPyBrowser::parseText (QString & pytext)
{
  QRegExp class_rx (QString ("^class [a-zA-Z0-9_,\\s\\(\\).]+:"));
  QRegExp function_rx (QString ("^def [a-zA-Z_]+[^#]*:"));
  QRegExp method_rx (QString ("[\\s]+def [a-zA-Z_]+[^#]*:"));

  int paren_i;
  QStringList lines = QStringList::split ("\n", pytext, TRUE);
  QStringList::Iterator iter;
  QString *line;
  QString class_name, method_name, function_name, class_sig, method_sig,
    function_sig;
  PyBrowseNode *last_class_node = NULL;
  PyBrowseNode *last_method_node = NULL;
  PyBrowseNode *last_function_node = NULL;

  Q3ValueList < QString > open_nodes;
  getOpenNodes (&open_nodes, class_root);
  getOpenNodes (&open_nodes, function_root);


  int line_no, state = 0;

  if (class_root != NULL)
    {
      delete class_root;
    }
  if (function_root != NULL)
    {
      delete function_root;
    }
  class_root =
    new PyBrowseNode (this, QString ("Classes"), i18n("Classes"),
		      PYOTHER);
  class_root->setPixmap (0, QPixmap (container_xpm));
  function_root =
    new PyBrowseNode (this, QString ("Globals"), i18n("Globals"),
		      PYOTHER);
  function_root->setPixmap (0, QPixmap (container_xpm));
  node_dict.insert (class_root->getQualifiedName (), class_root);
  node_dict.insert (function_root->getQualifiedName (), function_root);


  line_no = 0;

  for (iter = lines.begin(); iter != lines.end(); ++iter)
    {
      line_no++;
      line = &(*iter);
      if (class_rx.search(*line) >= 0)
	{
	  //KMessageBox::information(this, *line, QString("Found class on line %1").arg(line_no));
	  //strip out the beginning class and ending colon
	  class_sig = line->trimmed ().mid (6);
	  class_sig = class_sig.left (class_sig.length () - 1);
	  paren_i = class_sig.find ("(");
	  class_name = class_sig.left (paren_i);

	  last_class_node =
	    new PyBrowseNode (class_root, class_name, class_sig, PYCLASS);
	  last_class_node->setLine (line_no);
	  last_class_node->setClass (class_name);
	  node_dict.insert (last_class_node->getQualifiedName (),
			    last_class_node);
	  state = 1;
	}
      if ((method_rx.search(*line) >= 0) && (state == 1))
	{
	  //strip off the leading def and the ending colon
	  method_sig = line->trimmed ().mid (4);
	  method_sig = method_sig.left (method_sig.length () - 1);
	  paren_i = method_sig.find ("(");
	  method_name = method_sig.left (paren_i);
	  last_method_node =
	    new PyBrowseNode (last_class_node, method_name, method_sig,
			      PYMETHOD);
	  last_method_node->setLine (line_no);
	  last_method_node->setClass (last_class_node->getClass ());
	  node_dict.insert (last_method_node->getQualifiedName (),
			    last_method_node);
	}
      if ((function_rx.search(*line) >= 0))
	{
	  //KMessageBox::information(this, *line, QString("Found function on line %1").arg(line_no));
	  function_sig = line->trimmed ().mid (4);
	  function_sig = function_sig.left (function_sig.length () - 1);
	  paren_i = function_sig.find ("(");
	  function_name = function_sig.left (paren_i);
	  last_function_node =
	    new PyBrowseNode (function_root, function_name, function_sig,
			      PYFUNCTION);
	  last_function_node->setLine (line_no);
	  node_dict.insert (last_function_node->getQualifiedName (),
			    last_function_node);
	  state = 0;
	}
    }

    //now go through the list of old open nodes and open them in the new
    //tree.  For each node name in the open_nodes list, attempt to find that
    //node and in the new dict and open it.
    Q3ValueList<QString>::iterator it;
    PyBrowseNode *tmp_node;
    for (it=open_nodes.begin(); it != open_nodes.end(); ++it)
    {
    	tmp_node = node_dict[(*it)];
	if (tmp_node)
	{
		tmp_node->setOpen(1);
	}
    }

}


void
KPyBrowser::tip (const QPoint & p, QRect & r, QString & str)
{
  Q3ListViewItem *item = (Q3ListViewItem *) itemAt (p);
  if (item == NULL)
    {
      str = "";
      return;
    }
  r = itemRect (item);
  //r.setY(r.y() + 10);

  PyBrowseNode *browse_node = dynamic_cast < PyBrowseNode * >(item);

  if (browse_node)
    {
      if (r.isValid ())
	str = browse_node->getSig ();
      else
	str = "";
    }
  else
    {
      str = item->text (0);
    }
}

/////////////////////////////////////////////////////////////////////
// KateFileList::KFLToolTip implementation
/*
KPyBrowser::KPBToolTip::KPBToolTip (KPyBrowser * parent)
{
  b = parent;
}

void
KPyBrowser::KPBToolTip::maybeTip (const QPoint & p)
{
  QString str;
  QRect r;

  b->tip (p, r, str);

  if (!str.isEmpty () && r.isValid ())
    tip (r, str);
}
*/
