/* This file is part of the KDE libraries
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Charles Samuels <charles@kde.org>

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

#include "katecmds.h"

#include "katedocument.h"
#include "kateview.h"

#include <kdebug.h>

#include <qregexp.h>

namespace KateCommands
{

static void replace(QString &s, const QString &needle, const QString &with)
{
  int pos=0;
  while (1)
  {
    pos=s.find(needle, pos);
    if (pos==-1) break;
    s.replace(pos, needle.length(), with);
    pos+=with.length();
  }

}

static int backslashString(const QString &haystack, const QString &needle, int index)
{
  int len=haystack.length();
  int searchlen=needle.length();
  bool evenCount=true;
  while (index<len)
  {
    if (haystack[index]=='\\')
    {
      evenCount=!evenCount;
    }
    else
    {  // isn't a slash
      if (!evenCount)
      {
        if (haystack.mid(index, searchlen)==needle)
          return index-1;
      }
      evenCount=true;
    }
    index++;

  }

  return -1;
}


// exchange "\t" for the actual tab character, for example
static void exchangeAbbrevs(QString &str)
{
  // the format is (findreplace)*[nullzero]
  const char *magic="a\x07t\t";

  while (*magic)
  {
    int index=0;
    char replace=magic[1];
    while ((index=backslashString(str, QChar(*magic), index))!=-1)
    {
      str.replace(index, 2, QChar(replace));
      index++;
    }
    magic++;
    magic++;
  }
}

QString SedReplace::sedMagic(QString textLine, const QString &find, const QString &repOld, bool noCase, bool repeat)
{

  QRegExp matcher(find, noCase);

  int start=0;
  while (start!=-1)
  {
    start=matcher.search(textLine, start);

    if (start==-1) break;

    int length=matcher.matchedLength();


    QString rep=repOld;

    // now set the backreferences in the replacement
    QStringList backrefs=matcher.capturedTexts();
    int refnum=1;

    QStringList::Iterator i = backrefs.begin();
    ++i;

    for (; i!=backrefs.end(); ++i)
    {
      // I need to match "\\" or "", but not "\"
      QString number=QString::number(refnum);

      int index=0;
      while (index!=-1)
      {
        index=backslashString(rep, number, index);
        if (index>=0)
        {
          rep.replace(index, 2, *i);
          index+=(*i).length();
        }
      }

      refnum++;
    }

    replace(rep, "\\\\", "\\");
    replace(rep, "\\/", "/");

    textLine.replace(start, length, rep);
    if (!repeat) break;
    start+=rep.length();
  }


  return textLine;
}


static void setLineText(Kate::View *view, int line, const QString &text)
{
  if (view->getDoc()->insertLine(line, text))
    view->getDoc()->removeLine(line+1);
}

bool SedReplace::exec (Kate::View *view, const QString &cmd, QString &)
{
  kdDebug(13010)<<"SedReplace::execCmd()"<<endl;

  if (QRegExp("[$%]?s/.+/.*/[ig]*").search(cmd, 0)==-1)
    return false;

  bool fullFile=cmd[0]=='%';
  bool noCase=cmd[cmd.length()-1]=='i' || cmd[cmd.length()-2]=='i';
  bool repeat=cmd[cmd.length()-1]=='g' || cmd[cmd.length()-2]=='g';
  bool onlySelect=cmd[0]=='$';


  QRegExp splitter("^[$%]?s/((?:[^\\\\/]|\\\\.)*)/((?:[^\\\\/]|\\\\.)*)/[ig]*$");
  if (splitter.search(cmd)<0) return false;

  QString find=splitter.cap(1);
  kdDebug(13010)<< "SedReplace: find=" << find.latin1() <<endl;

  QString replace=splitter.cap(2);
  exchangeAbbrevs(replace);
  kdDebug(13010)<< "SedReplace: replace=" << replace.latin1() <<endl;


  if (fullFile)
  {
    int numLines=view->getDoc()->numLines();
    for (int line=0; line < numLines; line++)
    {
      QString text=view->getDoc()->textLine(line);
      text=sedMagic(text, find, replace, noCase, repeat);
      setLineText(view, line, text);
    }
  }
  else if (onlySelect)
  {
    // Not implemented
  }
  else
  { // just this line
    QString textLine=view->currentTextLine();
    int line=view->cursorLine();
    textLine=sedMagic(textLine, find, replace, noCase, repeat);
    setLineText(view, line, textLine);
  }
  return true;
}

bool Character::exec (Kate::View *view, const QString &_cmd, QString &)
{
  QString cmd = _cmd;

  // hex, octal, base 9+1
  QRegExp num("^char: *(0?x[0-9A-Fa-f]{1,4}|0[0-7]{1,6}|[0-9]{1,3})$");
  if (num.search(cmd)==-1) return false;

  cmd=num.cap(1);

  // identify the base

  unsigned short int number=0;
  int base=10;
  if (cmd[0]=='x' || cmd.left(2)=="0x")
  {
    cmd.replace(QRegExp("^0?x"), "");
    base=16;
  }
  else if (cmd[0]=='0')
    base=8;
  bool ok;
  number=cmd.toUShort(&ok, base);
  if (!ok || number==0) return false;
  if (number<=255)
  {
    char buf[2];
    buf[0]=(char)number;
    buf[1]=0;
    view->insertText(QString(buf));
  }
  else
  { // do the unicode thing
    QChar c(number);
    view->insertText(QString(&c, 1));
  }

  return true;
}

bool Goto::exec (Kate::View *view, const QString &cmd, QString &)
{
  if (cmd.left(5) != "goto:")
    return false;

  view->gotoLineNumber (cmd.mid(5, cmd.length()-5).toInt());

  return true;
}

bool Date::exec (Kate::View *view, const QString &cmd, QString &)
{
  if (cmd.left(4) != "date")
    return false;

  if (QDateTime::currentDateTime().toString(cmd.mid(5, cmd.length()-5)).length() > 0)
    view->insertText(QDateTime::currentDateTime().toString(cmd.mid(5, cmd.length()-5)));
  else
    view->insertText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

  return true;
}

}

// vim: noet

