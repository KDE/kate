/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef _KATE_DOCUMENT_INCLUDE_
#define _KATE_DOCUMENT_INCLUDE_

#include <qptrlist.h>
#include <kaction.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/undointerface.h>
#include <ktexteditor/cursorinterface.h>
#include <ktexteditor/selectioninterface.h>
#include <ktexteditor/blockselectioninterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/highlightinginterface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/markinterfaceextension.h>
#include <ktexteditor/wordwrapinterface.h>
#include <ktexteditor/printinterface.h>

namespace Kate
{

class View;

class Cursor : public KTextEditor::Cursor
{
  public:
    Cursor () { ; };
    virtual ~Cursor () { ; };
};

class ConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  public:
    ConfigPage ( QWidget *parent=0, const char *name=0 ) : KTextEditor::ConfigPage (parent, name) { ; };
    virtual ~ConfigPage () { ; };

  public slots:
    virtual void apply () { ; };
    virtual void reload () { ; };  
    virtual void reset () {};
    virtual void defaults () {};};

class ActionMenu : public KActionMenu
{
  Q_OBJECT

  public:
    ActionMenu ( const QString& text, QObject* parent = 0, const char* name = 0 )
      : KActionMenu(text, parent, name) { ; };
    virtual ~ActionMenu () { ; };

  public:
    virtual void updateMenu (class Document *) = 0;
};

/** This interface provides access to the Kate Document class.
*/
class Document : public KTextEditor::Document, public KTextEditor::EditInterface,
                     public KTextEditor::UndoInterface, public KTextEditor::CursorInterface,
                     public KTextEditor::SelectionInterface, public KTextEditor::SearchInterface,
                     public KTextEditor::HighlightingInterface, public KTextEditor::BlockSelectionInterface,
                     public KTextEditor::ConfigInterface, public KTextEditor::MarkInterface,
                     public KTextEditor::PrintInterface, public KTextEditor::WordWrapInterface,
                     public KTextEditor::MarkInterfaceExtension
/*GRRR !!! Never do that again. Declaring interfaces within interfaces, where it is not really necessary.
	The above inheritage just makes it completely impossible to enhance the ktexteditor interface in a decent way*/

{
  Q_OBJECT

  public:
    Document ();
    virtual ~Document ();

  public:
    /**
     * Checks if the file on disk is newer than document contents.
     * If forceReload is true, the document is reloaded without asking the user,
     * otherwise [default] the user is asked what to do.
     */
    virtual void isModOnHD(bool =false) { ; };

    /**
     * Returns the document name.
     */
    virtual QString docName () { return 0L; };

    /**
     * Sets the document name.
     */
    virtual void setDocName (QString ) { ; };

    virtual ActionMenu *hlActionMenu (const QString& , QObject* =0, const char* = 0) = 0;
    virtual ActionMenu *exportActionMenu (const QString& , QObject* =0, const char* = 0) = 0;

  public slots:
    // clear buffer/filename - update the views
    virtual void flush () { ; };

    /**
     * Reloads the current document from disk if possible
     */
    virtual void reloadFile() = 0;

    /**
     * @deprecated Spellchecking is implemented as a plugin.
     * This does nothing.
     */
    // FIXME: Remove when BIC allowed.
    virtual void spellcheck() {};

    virtual void exportAs(const QString &) = 0;

    virtual void applyWordWrap () = 0;

    
  public:
    virtual void setWordWrap (bool ) = 0;
    virtual bool wordWrap () = 0;

    virtual void setWordWrapAt (uint) = 0;
    virtual uint wordWrapAt () = 0;


    virtual void setEncoding (const QString &e) = 0;
    virtual QString encoding() const = 0;
        
  public:
    virtual ConfigPage *colorConfigPage (QWidget *) = 0;
    virtual ConfigPage *fontConfigPage (QWidget *) = 0;
    virtual ConfigPage *indentConfigPage (QWidget *) = 0;
    virtual ConfigPage *selectConfigPage (QWidget *) = 0;
    virtual ConfigPage *editConfigPage (QWidget *) = 0;
    virtual ConfigPage *keysConfigPage (QWidget *) = 0;
    /** @deprecated */
    // FIXME: Remove when BIC allowed.
    virtual ConfigPage *kSpellConfigPage (QWidget *) { return 0L; }
    virtual ConfigPage *hlConfigPage (QWidget *) = 0;
    
  public:
    virtual uint configFlags () = 0;
    virtual void setConfigFlags (uint flags) = 0;

    // Flags for katedocument config !
    enum ConfigFlags
    {
      cfAutoIndent= 0x1,
      cfBackspaceIndents= 0x2,
      cfWordWrap= 0x4,
      cfReplaceTabs= 0x8,
      cfRemoveSpaces = 0x10,
      cfWrapCursor= 0x20,
      cfAutoBrackets= 0x40,
      cfPersistent= 0x80,
      cfKeepSelection= 0x100,
      cfDelOnInput= 0x400,
      cfXorSelect= 0x800,
      cfOvr= 0x1000,
      cfMark= 0x2000,
      cfKeepIndentProfile= 0x8000,
      cfKeepExtraSpaces= 0x10000,
      cfTabIndents= 0x80000,
      cfShowTabs= 0x200000,
      cfSpaceIndent= 0x400000,
      cfSmartHome = 0x800000
    };
};

};

#endif
