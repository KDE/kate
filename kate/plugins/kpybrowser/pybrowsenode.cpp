/***************************************************************************
                          pybrowsenode.cpp  -  description
                             -------------------
    begin                : Mon Aug 27 2001
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

#include "pybrowsenode.h"
//Added by qt3to4:
#include <QPixmap>

static const char *py_class_xpm[] = {
  "16 16 10 1",
  "       c None",
  ".      c #000000",
  "+      c #A4E8FC",
  "@      c #24D0FC",
  "#      c #001CD0",
  "$      c #0080E8",
  "%      c #C0FFFF",
  "&      c #00FFFF",
  "*      c #008080",
  "=      c #00C0C0",
  "     ..         ",
  "    .++..       ",
  "   .+++@@.      ",
  "  .@@@@@#...    ",
  "  .$$@@##.%%..  ",
  "  .$$$##.%%%&&. ",
  "  .$$$#.&&&&&*. ",
  "   ...#.==&&**. ",
  "   .++..===***. ",
  "  .+++@@.==**.  ",
  " .@@@@@#..=*.   ",
  " .$$@@##. ..    ",
  " .$$$###.       ",
  " .$$$##.        ",
  "  ..$#.         ",
  "    ..          "
};

static const char *py_method_xpm[] = {
  "16 16 5 1",
  "       c None",
  ".      c #000000",
  "+      c #FCFC80",
  "@      c #E0BC38",
  "#      c #F0DC5C",
  "                ",
  "                ",
  "                ",
  "          ..    ",
  "         .++..  ",
  "        .+++++. ",
  "       .+++++@. ",
  "    .. .##++@@. ",
  "   .++..###@@@. ",
  "  .+++++.##@@.  ",
  " .+++++@..#@.   ",
  " .##++@@. ..    ",
  " .###@@@.       ",
  " .###@@.        ",
  "  ..#@.         ",
  "    ..          "
};

static const char *py_function_xpm[] = {
  "16 16 6 1",
  "       c None",
  ".      c #240000",
  "+      c #000000",
  "@      c #FCFC80",
  "#      c #E0BC38",
  "$      c #F0DC5C",
  " ........       ",
  ".        .      ",
  ".        .      ",
  " ........ ++    ",
  "         +@@++  ",
  "        +@@@@@+ ",
  "       +@@@@@#+ ",
  "    ++ +$$@@##+ ",
  "   +@@++$$$###+ ",
  "  +@@@@@+$$##+  ",
  " +@@@@@#++$#+   ",
  " +$$@@##+ ++    ",
  " +$$$###+       ",
  " +$$$##+        ",
  "  ++$#+         ",
  "    ++          "
};


PyBrowseNode::PyBrowseNode (Q3ListViewItem * parent, const QString &name,
			    const QString &signature, int nodeType):
Q3ListViewItem (parent, signature)
{
  init (name, signature, nodeType);
}

PyBrowseNode::PyBrowseNode (Q3ListView * parent, const QString &name,
			    const QString &signature, int nodeType):
Q3ListViewItem (parent, signature)
{
  init (name, signature, nodeType);
}

void
PyBrowseNode::init (const QString &a_name, const QString &a_signature, int nodeType)
{
  node_type = nodeType;
  if (nodeType == PYCLASS)
    setPixmap (0, QPixmap (py_class_xpm));
  if (nodeType == PYMETHOD)
    setPixmap (0, QPixmap (py_method_xpm));
  if (nodeType == PYFUNCTION)
    setPixmap (0, QPixmap (py_function_xpm));

  name = a_name;
  signature = a_signature;
}

PyBrowseNode::~PyBrowseNode ()
{
  setPixmap (0, QPixmap (py_class_xpm));
}

void
PyBrowseNode::setName (const QString &a_name)
{
  name = a_name;
  setText (0, name);
}

QString
PyBrowseNode::getName ()const
{
  return name;
}

void
PyBrowseNode::setSig (const QString &a_signature)
{
  signature = a_signature;

}

QString
PyBrowseNode::getSig ()const
{
  return signature;
}

void
PyBrowseNode::setLine (int a_line)
{
  line = a_line;
}

int
PyBrowseNode::getLine ()const
{
  return line;
}


void
PyBrowseNode::setType (int type)
{
  node_type = type;
}

int
PyBrowseNode::getType ()const
{
  return node_type;
}

void
PyBrowseNode::setClass (const QString &a_method_class)
{
  node_class = a_method_class;
}

QString
PyBrowseNode::getClass ()const
{
  return node_class;
}

QString
PyBrowseNode::getQualifiedName ()const
{
  if (node_type == PYCLASS)
    return node_class;
  if (node_type == PYMETHOD)
    return node_class + "::" + name;
   if (node_type == PYFUNCTION)
      return name;
   return name;
}
