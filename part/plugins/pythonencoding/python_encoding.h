/* This file is part of the KDE libraries
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
      [katedocument.cpp, LGPL v2 only]

================RELICENSED=================

    This file is part of the KDE libraries
    Copyright (C) 2008 Joseph Wenninger <jowenn@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KTEXTEDITOR_PYTHON_ENCODING
#define KTEXTEDITOR_PYTHON_ENCODING

#include <ktexteditor/loadsavefiltercheckplugin.h>
#include <ktexteditor/view.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <QList>
#include <QVariant>

class KTextEditorPythonEncodingCheck: public KTextEditor::LoadSaveFilterCheckPlugin {
  Q_OBJECT
  public:
    KTextEditorPythonEncodingCheck(QObject *parent, QList<QVariant>):KTextEditor::LoadSaveFilterCheckPlugin(parent){interpreterLine=QRegExp(QString("#!.*$"));}
    virtual ~KTextEditorPythonEncodingCheck(){}
    virtual bool postSaveFilterCheck(KTextEditor::Document *document,bool saveas) {return true;}
    virtual bool preSavePostDialogFilterCheck(KTextEditor::Document *document)
    {
      kDebug(13020)<<"KTextEditorPythonEncodingCheck::preSavePostDialogCheck";
      //QString codec=document->config()->codec()->name().toLower();
      QString codec=document->encoding().toLower();
      codec.replace(' ','-');
//	"#\s*-\*\-\s*coding[:=]\s*([-\w.]+)\s*-\*-\s*$"
      bool firstIsInterpreter=false;
      QRegExp encoding_regex(QString("#\\s*-\\*\\-\\s*coding[:=]\\s*%1\\s*-\\*-\\s*$").arg(codec));
      bool correctencoding=false;
      if (document->lines()>0)
      {
        if (encoding_regex.exactMatch(document->line(0))) correctencoding=true;
        else if (document->lines()>1) {
          if (interpreterLine.exactMatch(document->line(0)))
          {
            firstIsInterpreter=true;
            if (encoding_regex.exactMatch(document->line(1))) correctencoding=true;
          }
        }
      }
      if (!correctencoding) {
        QString addLine(QString("# -*- coding: %1 -*-").arg(codec));
        int what=KMessageBox::warningYesNoCancel (document->activeView()
        , i18n ("You are trying to save a python file as non ASCII, without specifiying a correct source encoding line for encoding \"%1\"", codec)
        , i18n ("No encoding header")
        , KGuiItem(i18n("Insert: %1",addLine))
        , KGuiItem(i18n("Save Nevertheless"))
        , KStandardGuiItem::cancel(), "OnSave-WrongPythonEncodingHeader");
        switch (what) {
          case KMessageBox::Yes:
          {
            int line=firstIsInterpreter?1:0;
            QRegExp checkReplace_regex(QString("#\\s*-\\*\\-\\s*coding[:=]\\s*[-\\w]+\\s*-\\*-\\s*$"));
            if (checkReplace_regex.exactMatch(document->line(line)))
              document->removeLine(line);
            document->insertLine(line,addLine);
            break;
          }
          case KMessageBox::No:
            return true;
            break;
          default:
            return false;
            break;
        }
      }
      return true;
    }
    virtual void postLoadFilter(KTextEditor::Document*){    }
    private:
      QRegExp interpreterLine;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;