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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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
#include <ktexteditor/configinterfaceextension.h>
#include <kxmlguiclient.h>

#include <qevent.h>
#include <qobject.h>
#include <qvaluelist.h>

class DocWordCompletionPlugin
  : public KTextEditor::Plugin
  , public KTextEditor::PluginViewInterface
  , public KTextEditor::ConfigInterfaceExtension
{
  Q_OBJECT

  public:
    DocWordCompletionPlugin( QObject *parent = 0,
                            const char* name = 0,
                            const QStringList &args = QStringList() );
    virtual ~DocWordCompletionPlugin() {};

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

    void readConfig();
    void writeConfig();

    // ConfigInterfaceExtention
    uint configPages() const { return 1; };
    KTextEditor::ConfigPage * configPage( uint number, QWidget *parent, const char *name );
    QString configPageName( uint ) const;
    QString configPageFullName( uint ) const;
    QPixmap configPagePixmap( uint, int ) const;

    uint treshold() const { return m_treshold; };
    void setTreshold( uint t ) { m_treshold = t; };
    bool autoPopupEnabled() const { return m_autopopup; };
    void setAutoPopupEnabled( bool enable ) { m_autopopup = enable; };


  private:
    QPtrList<class DocWordCompletionPluginView> m_views;
    uint m_treshold;
    bool m_autopopup;

};

class DocWordCompletionPluginView
   : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    DocWordCompletionPluginView( uint treshold=3, bool autopopup=true, KTextEditor::View *view=0,
                               const char *name=0 );
    ~DocWordCompletionPluginView() {};

    void settreshold( uint treshold );

  private slots:
    void completeBackwards();
    void completeForwards();

    void popupCompletionList( QString word=QString::null );
    void autoPopupCompletionList();
    void toggleAutoPopup();

    void slotVariableChanged( const QString &, const QString & );

  private:
    void complete( bool fw=true );
    QString word();
    QValueList<KTextEditor::CompletionEntry> allMatches( const QString &word );
    KTextEditor::View *m_view;
    struct DocWordCompletionPluginViewPrivate *d;
};

class DocWordCompletionConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT
  public:
    DocWordCompletionConfigPage( DocWordCompletionPlugin *completion, QWidget *parent, const char *name );
    virtual ~DocWordCompletionConfigPage() {};

    virtual void apply();
    virtual void reset();
    virtual void defaults();

  private:
    DocWordCompletionPlugin *m_completion;
    class QCheckBox *cbAutoPopup;
    class QSpinBox *sbAutoPopup;
    class QLabel *lSbRight;
};

#endif // _DocWordCompletionPlugin_h_
