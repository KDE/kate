/* This file is part of the KDE libraries
   Copyright (C) 2004-2005 Anders Lund <anders@alweb.dk>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef __KATE_SEARCH_H__
#define __KATE_SEARCH_H__

#include <ktexteditor/range.h>
#include <ktexteditor/commandinterface.h>

#include <kdialogbase.h>

#include <qstring.h>
#include <qregexp.h>
#include <qstringlist.h>

class KateView;
class KateDocument;

class KActionCollection;

class KateSearch : public QObject
{
  Q_OBJECT

  friend class KateDocument;

  private:
    class SearchFlags
    {
      public:
        bool caseSensitive     :1;
        bool wholeWords        :1;
        bool fromBeginning     :1;
        bool backward          :1;
        bool selected          :1;
        bool prompt            :1;
        bool replace           :1;
        bool finished          :1;
        bool regExp            :1;
        bool useBackRefs       :1;
    };

    class SConfig
    {
      public:
        SearchFlags flags;
        KTextEditor::Cursor cursor;
        KTextEditor::Cursor wrappedEnd; // after wraping around, search/replace until here
        bool wrapped; // have we allready wrapped around ?
        bool showNotFound; // pop up annoying dialogs?
        int matchedLength;
        KTextEditor::Range selection;
    };

  public:
    enum Dialog_results {
      srCancel = KDialogBase::Cancel,
      srAll = KDialogBase::User1,
      srLast = KDialogBase::User2,
      srNo = KDialogBase::User3,
      srYes = KDialogBase::Ok
    };

  public:
    KateSearch( KateView* );
    ~KateSearch();

    void createActions( KActionCollection* );

  public Q_SLOTS:
    void find();
    /**
     * Search for @p pattern given @p flags
     * This is for the commandline "find", and is forwarded by
     * KateView.
     * @param pattern string or regex pattern to search for.
     * @param flags a OR'ed combination of KFindDialog::Options
     * @param add wether this string should be added to the recent search list
     * @param shownotfound wether to pop up "Not round: PATTERN" when that happens.
     * That must now be explicitly required -- the find dialog does, but the commandline
     * incremental search does not.
     */
    void find( const QString &pattern, long flags, bool add=true, bool shownotfound=false );
    void replace();
    /**
     * Replace @p pattern with @p replacement given @p flags.
     * This is for the commandline "replace" and is forwarded
     * by KateView.
     * @param pattern string or regular expression to search for
     * @param replacement Replacement string.
     * @param flags OR'd combination of KFindDialog::Options
     */
    void replace( const QString &pattern, const QString &replacement, long flags );
    void findAgain( bool back );

  private Q_SLOTS:
    void replaceSlot();
    void slotFindNext() { findAgain( false ); }
    void slotFindPrev() { findAgain( true );  }

  private:
    static void addToList( QStringList&, const QString& );
    static void addToSearchList( const QString& s )  { addToList( s_searchList, s ); }
    static void addToReplaceList( const QString& s ) { addToList( s_replaceList, s ); }
    static QStringList s_searchList; ///< recent patterns
    static QStringList s_replaceList; ///< recent replacement strings
    static QString s_pattern; ///< the string to search for

    void search( SearchFlags flags );
    void wrapSearch();
    bool askContinue();

    void findAgain();
    void promptReplace();
    void replaceAll();
    void replaceOne();
    void skipOne();

    QString getSearchText();
    KTextEditor::Cursor getCursor();
    bool doSearch( const QString& text );
    void exposeFound( KTextEditor::Cursor &cursor, int slen );

    inline KateView* view()    { return m_view; }
    inline KateDocument* doc() { return m_doc;  }

    KateView*     m_view;
    KateDocument* m_doc;

    SConfig s;

    int                 m_resultIndex;

    int           replaces;
    QDialog*      replacePrompt;
    QString m_replacement;
    QRegExp m_re;
};

/**
 * simple replace prompt dialog
 */
class KateReplacePrompt : public KDialogBase
{
  Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent parent widget for the dialog
     */
    KateReplacePrompt(QWidget *parent);

  Q_SIGNALS:
    /**
     * button clicked
     */
    void clicked();

  protected Q_SLOTS:
    /**
     * ok pressed
     */
    void slotOk ();

    /**
     * close pressed
     */
    void slotClose ();

    /**
     * replace all pressed
     */
    void slotUser1 ();

    /**
     * last pressed
     */
    void slotUser2 ();

    /**
     * Yes pressed
     */
    void slotUser3 ();

    /**
     * dialog done
     * @param result dialog result
     */
    void done (int result);
};

class SearchCommand : public KTextEditor::Command, public KTextEditor::CommandExtension
{
  public:
    SearchCommand() : m_ifindFlags(0) {;}
    bool exec(class KTextEditor::View *view, const QString &cmd, QString &errorMsg);
    bool help(class KTextEditor::View *, const QString &, QString &);
    const QStringList &cmds();
    bool wantsToProcessText( const QString &/*cmdname*/ );
    void processText( KTextEditor::View *, const QString& );

    virtual void flagCompletions( QStringList& ) {}
    virtual KCompletion *completionObject( KTextEditor::View *, const QString & ) { return 0; }

  private:
    /**
     * set up properties for incremental find
     */
    void ifindInit( const QString &cmd );
    /**
     * clear properties for incremental find
     */
    void ifindClear();

    long m_ifindFlags;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
