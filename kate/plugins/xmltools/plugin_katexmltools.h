 /***************************************************************************
    pluginKatexmltools.cpp
    copyright            : (C) 2001-2002 by Daniel Naber
    email                : daniel.naber@t-online.de
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or ( at your option ) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

#ifndef PLUGIN_KATEXMLTOOLS_H
#define PLUGIN_KATEXMLTOOLS_H

#include "pseudo_dtd.h"

#include <q3dict.h>
#include <qstring.h>
#include <q3listbox.h>
#include <q3progressdialog.h>
#include <q3intdict.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <Q3PtrList>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>

#include <kcombobox.h>
#include <kdialog.h>

class PluginKateXMLTools : public Kate::Plugin, Kate::PluginViewInterface
{

  Q_OBJECT

  public:

    PluginKateXMLTools( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~PluginKateXMLTools();
    void addView ( Kate::MainWindow *win );
    void removeView( Kate::MainWindow *win );
	void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
	void loadGeneralConfig(KConfig* config, const QString& groupPrefix);

  public slots:

    void getDTD();

    void slotInsertElement();
    void slotCloseElement();
    void filterInsertString( KTextEditor::CompletionItem *ce, QString *str );
    void completionDone( KTextEditor::CompletionItem completionEntry );
    void completionAborted();

    void slotFinished( KJob *job );
    void slotData( KIO::Job *, const QByteArray &data );

    void backspacePressed();
    void emptyKeyEvent();
    void keyEvent( int, int, const QString & );

    /// Connected to the document manager, to manage the dtd collection.
    void slotDocumentDeleted( uint n );

  protected:

    static QStringList sortQStringList( QStringList list );
    //bool eventFilter( QObject *object, QEvent *event );

    QString insideTag( KTextEditor::View &kv );
    QString insideAttribute( KTextEditor::View &kv );

    bool isOpeningTag( QString tag );
    bool isClosingTag( QString tag );
    bool isEmptyTag( QString tag );
    bool isQuote( QString ch );

    QString getParentElement( KTextEditor::View &view, bool ignoreSingleBracket );

    enum Mode {none, entities, attributevalues, attributes, elements};
    enum PopupMode {noPopup, tagname, attributename, attributevalue, entityname};

    Q3ValueList<KTextEditor::CompletionItem> stringListToCompletionEntryList( QStringList list );

    /// Assign the PseudoDTD @p dtd to the Kate::Document @p doc
    void assignDTD( PseudoDTD *dtd, KTextEditor::Document *doc );

    /// temporary placeholder for the metaDTD file
    QString m_dtdString;
    /// temporary placeholder for the document to assign a DTD to while the file is loaded
    KTextEditor::Document *m_docToAssignTo;
    /// URL of the last loaded meta DTD
    QString m_urlString;

    int m_lastLine, m_lastCol;
    QStringList m_lastAllowed;
    int m_popupOpenCol;

    Mode m_mode;
    int m_correctPos;

    // code completion stuff:
    KTextEditor::CodeCompletionInterface* m_codeInterface;

    /// maps KTE::Document::docNumber -> DTD
    Q3IntDict<PseudoDTD> m_docDtds;

    /// maps DTD filename -> DTD
    Q3Dict<PseudoDTD> m_dtds;

    Q3PtrList<class PluginView> m_views;

    void connectSlots( KTextEditor::View *kv );
    void disconnectSlots( KTextEditor::View *kv );

    Kate::DocumentManager *m_documentManager;
};

class InsertElement : public KDialog
{

  Q_OBJECT

  public:
    InsertElement( QWidget *parent, const char *name );
    ~InsertElement();
    QString showDialog( QStringList &completions );
  private slots:
    void slotHistoryTextChanged( const QString& );

};

#endif // PLUGIN_KATEXMLTOOLS_H

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
