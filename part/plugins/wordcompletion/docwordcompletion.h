/*

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
#include <kxmlguiclient.h>

#include <qevent.h>
#include <qobject.h>
#include <qvaluelist.h>

class DocWordCompletionPlugin
  : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface
{
  Q_OBJECT

  public:
    DocWordCompletionPlugin( QObject *parent = 0,
                            const char* name = 0,
                            const QStringList &args = QStringList() );
    virtual ~DocWordCompletionPlugin() {};

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

  private:
    QPtrList<class DocWordCompletionPluginView> m_views;

};

class DocWordCompletionPluginView
   : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    DocWordCompletionPluginView( KTextEditor::View *view,
                               const char *name=0 );
    ~DocWordCompletionPluginView() {};

  private slots:
    void completeBackwards();
    void completeForwards();

    void popupCompletionList( QString word=QString::null );
    void autoPopupCompletionList();
    void toggleAutoPopup();

  private:
    void complete( bool fw=true );
    QString word();
    QValueList<KTextEditor::CompletionEntry> allMatches( const QString &word );
    KTextEditor::View *m_view;
    struct DocWordCompletionPluginViewPrivate *d;
};

#endif // _DocWordCompletionPlugin_h_
