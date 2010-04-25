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

#include <qstring.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include <kcombobox.h>
#include <kdialog.h>


class PluginKateXMLTools : public Kate::Plugin
{
  Q_OBJECT

  public:
    explicit PluginKateXMLTools( QObject* parent = 0, const QStringList& = QStringList() );
    ~PluginKateXMLTools();
    Kate::PluginView *createView(Kate::MainWindow *mainWindow);
};

class PluginKateXMLToolsCompletionModel : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface3
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

  public:
    PluginKateXMLToolsCompletionModel( QObject *parent );
    virtual ~PluginKateXMLToolsCompletionModel();

    virtual QVariant data(const QModelIndex &idx, int role) const;

    virtual bool shouldStartCompletion( KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position );

  public slots:

    void getDTD();

    void slotInsertElement();
    void slotCloseElement();

    void slotFinished( KJob *job );
    void slotData( KIO::Job *, const QByteArray &data );

    void backspacePressed();
    void emptyKeyEvent();
    void completionInvoked( KTextEditor::View *kv, const KTextEditor::Range &range, InvocationType invocationType );

    /// Connected to the document manager, to manage the dtd collection.
    void slotDocumentDeleted( KTextEditor::Document *doc );

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

    /// Assign the PseudoDTD @p dtd to the Kate::Document @p doc
    void assignDTD( PseudoDTD *dtd, KTextEditor::Document *doc );

    /// temporary placeholder for the metaDTD file
    QString m_dtdString;
    /// temporary placeholder for the document to assign a DTD to while the file is loaded
    KTextEditor::Document *m_docToAssignTo;
    /// URL of the last loaded meta DTD
    QString m_urlString;

    int m_lastLine, m_lastCol;
    QStringList m_allowed;
    int m_popupOpenCol;

    Mode m_mode;
    int m_correctPos;

    // code completion stuff:
    KTextEditor::CodeCompletionInterface* m_codeInterface;

    /// maps KTE::Document -> DTD
    QHash<KTextEditor::Document *, PseudoDTD *> m_docDtds;

    /// maps DTD filename -> DTD
    QHash<QString, PseudoDTD *> m_dtds;
};

class PluginKateXMLToolsView : public Kate::PluginView, public Kate::XMLGUIClient
{
  Q_OBJECT

  public:

    explicit PluginKateXMLToolsView( Kate::MainWindow *win);
    virtual ~PluginKateXMLToolsView();

  protected:
    PluginKateXMLToolsCompletionModel m_model;
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
