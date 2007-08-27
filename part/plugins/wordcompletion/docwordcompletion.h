/*
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

    ---
    file: docwordcompletion.h

    KTextEditor plugin to autocompletion with document words.
    Copyright Anders Lund <anders.lund@lund.tdcadsl.dk>, 2003

    The following completion methods are supported:
    * Completion with bigger matching words in
      either direction (backward/forward).
    * NOT YET Pop up a list of all bigger matching words in document

*/

#ifndef _DocWordCompletionPlugin_h_
#define _DocWordCompletionPlugin_h_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/configpage.h>
#include <kxmlguiclient.h>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QList>

#include <kdebug.h>

class DocWordCompletionModel
  : public KTextEditor::CodeCompletionModel
{
  Q_OBJECT
  public:
    DocWordCompletionModel( QObject *parent );
    ~DocWordCompletionModel();

    void saveMatches( KTextEditor::View* view,
                            const KTextEditor::Range& range);

    int rowCount ( const QModelIndex & parent ) const;

    QVariant data(const QModelIndex& index, int role) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;

    const QStringList allMatches( KTextEditor::View *view, const KTextEditor::Range &range, int minAdditionalLength = 1 ) const;

  private:
    QStringList m_matches;
};

class DocWordCompletionPlugin
  : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit DocWordCompletionPlugin( QObject *parent,
                            const QVariantList &args );
    virtual ~DocWordCompletionPlugin(){kDebug()<<"~DocWordCompletionPlugin";}

    static DocWordCompletionPlugin *self() { return plugin; }

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

    void readConfig();
    void writeConfig();

    virtual void readConfig (KConfig *) {}
    virtual void writeConfig (KConfig *) {}

    uint treshold() const;
    void setTreshold( uint t );
    bool autoPopupEnabled() const;
    void setAutoPopupEnabled( bool enable );
  private:
    static DocWordCompletionPlugin *plugin;
    QList<class DocWordCompletionPluginView*> m_views;
    uint m_treshold;
    bool m_autopopup;
    DocWordCompletionModel *m_dWCompletionModel;

};

class DocWordCompletionPluginView
   : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    explicit DocWordCompletionPluginView( uint treshold=3,
                                 bool autopopup=true,
                                 KTextEditor::View *view=0,
                                 DocWordCompletionModel *completionModel=0 );
    ~DocWordCompletionPluginView();

    void setTreshold( uint treshold );
    void setAutoPopupEnabled( bool enable );

  public Q_SLOTS:
    void toggleAutoPopup();

  private Q_SLOTS:
    void completeBackwards();
    void completeForwards();
    void shellComplete();

    void popupCompletionList();
    void autoPopupCompletionList();

    void slotVariableChanged(KTextEditor::Document*, const QString &, const QString & );

  private:
    void complete( bool fw=true );

    const QString word() const;
    const KTextEditor::Range range() const;

    QString findLongestUnique( const QStringList &matches, int lead ) const;

    KTextEditor::View *m_view;
    DocWordCompletionModel *m_dWCompletionModel;
    struct DocWordCompletionPluginViewPrivate *d;
};

#endif // _DocWordCompletionPlugin_h_
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
