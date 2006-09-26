/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>
   Copyright (C) 2004 >Anders Lund <anders@alweb.dk> (KateVarIndent class)
   Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de> (basic support for config page)

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

#ifndef __KATE_AUTO_INDENT_H__
#define __KATE_AUTO_INDENT_H__

#include "katecursor.h"
#include "kateconfig.h"
#include "katejscript.h"

#include <kactionmenu.h>

class KateDocument;
class KateIndentJScript;

/**
 * This widget will be embedded into a modal dialog when clicking
 * the "Configure..." button in the indentation config page.
 * To add a config page for an indenter there are several todos:
 * - Derive a class from this class. This widget will be embedded into
 *   the config dialog.
 * - Override the slot @p apply(), which is called when the configuration
 *   needs to be saved.
 * - Override @p KateAutoIndent::configPage() to return an instance of
 *   this dialog.
 * - Return @p true in @p KateAutoIndent::hasConfigPage() for the
 *   corresponding indenter id.
 */
class IndenterConfigPage : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Standard constructor
     * @param parent parent widget
     */
    IndenterConfigPage ( QWidget *parent=0 ) : QWidget(parent) {}
    virtual ~IndenterConfigPage () {}

  public Q_SLOTS:
    /**
     * Apply the changes. Save options here, use @p kapp->config() and
     * group [Kate Indenter MyIndenter].
     */
    virtual void apply () = 0;
};

/**
 * Provides Auto-Indent functionality for katepart.
 * This baseclass is a real dummy, does nothing beside remembering the document it belongs too,
 * only to have the object around
 */
class KateAutoIndent
{
  /*
   * Static methods to list indention modes
   */
  public:
    /**
     * List all possible modes by name
     * @return list of modes
     */
    static QStringList listModes ();

    /**
     * Return the mode name given the mode
     * @param mode mode index
     * @return name for this mode index
     */
    static QString modeName (int mode);

    /**
     * Return the mode description
     * @param mode mode index
     * @return mode index
     */
    static QString modeDescription (int mode);

    /**
     * Maps name -> index
     * @param name mode name
     * @return mode index
     */
    static uint modeNumber (const QString &name);

    /**
     * count of modes
     * @return number of existing modes
     */
    static int modeCount ();

    /**
     * Config page support
     * @param mode mode index
     * @return true, if the indenter @p mode has a configuration page
     */
    static bool hasConfigPage (int mode);

    /**
     * Support for a config page.
     * @return config page or 0 if not available.
     */
    static IndenterConfigPage* configPage(QWidget *parent, int mode);

  /*
   * Construction + Destruction
   */
  public:
    /**
     * Constructor, creates dummy indenter "None"
     * \param doc parent document
     */
    KateAutoIndent (KateDocument *doc);

    /**
     * Virtual Destructor for the baseclass
     */
    ~KateAutoIndent ();

  /*
   * Internal helper for the subclasses and itself
   */
  protected:
    /**
     * Produces a string with the proper indentation characters for its length.
     *
     * @param length The length of the indention in characters.
     * @return A QString representing @p length characters (factoring in tabs and spaces)
     */
    QString tabString (int length) const;

    /**
     * Change the indent of the specified line by the number of levels
     * specified by change.
     * positive values will indent more, negative values will unindent...
     * if relative is not true, the change will be used to set the indent level of the line
     * \param view the current active view
     * \param line line to change indent for
     * \param change change the indentation by given number of SPACES
     * \param relative is the change a relative change to the current indent level or should
     * the indent of the given line be set to the given indentation level
     */
    bool doIndent ( KateView *view, int line, int change, bool relative, bool keepExtraSpaces = false );

  public:
    /**
     * Switch indenter
     * Nop if already set to given mode
     * Otherwise switch to given indenter or to "None" if no suitable found...
     * @param name indention mode wanted
     */
    void setMode (const QString &name);

    /**
     * mode name
     */
    QString modeName () { return m_mode; }

    /**
     * Update indenter's configuration (indention width, etc.)
     * Is called in the updateConfig() of the document and after creation of the indenter...
     */
    void updateConfig ();

    /**
     * Function to provide the common indent/unindent/clean indent functionality to the document
     * This should be generic for all indenters, internally it uses the doIndent function,
     * in advance it keeps the "keep indent profile" option in mind
     * This works equal for all indenters, even for "none" or the scripts
     * \param view view to work on
     * \param range range of text to change indent for
     * \param change level of indents to add or remove, zero will still trigger cleaning of indentation
     * and removal of extra spaces, if option set
     * \return \e true on success, otherwise \e false
     */
    bool changeIndent (KateView *view, const KTextEditor::Range &range, int change);

    /**
     * The document requests the indenter to indent the given range of existing text.
     * This may happen to indent text pasted or to reindent existing text.
     * For "none" and "normal" this is a nop, for the scripts, the expression
     * will be asked for indent level for each line
     * \param view the view the user work at
     * \param range the range of text to indent...
     */
    void indent (KateView *view, const KTextEditor::Range &range);

    /**
     * The user typed some char, the indenter can react on this
     * '\n' will be send as char if the user wraps a line
     * \param view the view the user work at
     * \param position current cursor position, after the inserted char...
     * \param typedChar the inserted char
     */
    void userTypedChar (KateView *view, const KTextEditor::Cursor &position, QChar typedChar);

  protected:
    void scriptIndent (KateView *view, const KTextEditor::Cursor &position, QChar typedChar);

  /*
   * needed data
   */
  protected:
    KateDocument *doc; //!< the document the indenter works on
    int  tabWidth;     //!< The number of characters simulated for a tab
    int  indentWidth;  //!< The number of characters used when tabs are replaced by spaces
    bool  useSpaces;    //!< Should we use spaces or tabs to indent
    bool  keepProfile;  //!< Always try to honor the leading whitespace of lines already in the file
    bool  keepExtra;    //!< Keep indentation that is not on indentation boundaries
    QString m_mode;
    bool m_normal;
    KateIndentJScript *m_script;
};

/**
 * This action provides a list of available indenters and gets plugged
 * into the KateView's KActionCollection.
 */
class KateViewIndentationAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewIndentationAction(KateDocument *_doc, const QString& text, KActionCollection* parent = 0, const char* name = 0);

    ~KateViewIndentationAction(){;};

  private:
    KateDocument* doc;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setMode (QAction*);
};

class ScriptIndentConfigPage : public IndenterConfigPage
{
  Q_OBJECT

  public:
    ScriptIndentConfigPage ( QWidget *parent=0 );
    virtual ~ScriptIndentConfigPage ();

  public Q_SLOTS:
    /**
     * Apply changes.
     */
    virtual void apply ();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
