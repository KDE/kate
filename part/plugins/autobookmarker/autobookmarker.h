/*
    This library is free software you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    file: autobookmarker.h

    KTextEditor plugin to add bookmarks to documents.
    Copyright Anders Lund <anders.lund@lund.tdcadsl.dk>, 2003
*/

#ifndef _AUTOBOOKMARKER_H_
#define _AUTOBOOKMARKER_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/configinterfaceextension.h>

#include <kdialogbase.h>

#include <q3ptrlist.h>
//Added by qt3to4:
#include <QPixmap>

class AutoBookmarkEnt
{
  public:
  enum REFlags { CaseSensitive=1, MinimalMatching=2 };
  AutoBookmarkEnt(const QString &p=QString(),
                  const QStringList &f=QStringList(),
                  const QStringList &m=QStringList(),
                  int flags=1 );
  ~AutoBookmarkEnt(){}
  QString pattern;
  QStringList filemask;
  QStringList mimemask;
  int flags;
};

class AutoBookmarker
  : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface,
    public KTextEditor::ConfigInterfaceExtension
{
  Q_OBJECT
  public:
    AutoBookmarker( QObject *parent = 0,
                        const char* name = 0,
                        const QStringList &args = QStringList() );
    virtual ~AutoBookmarker() {}

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

    // ConfigInterfaceExtention
    uint configPages() const { return 1; };
    KTextEditor::ConfigPage * configPage( uint number, QWidget *parent, const char *name );
    QString configPageName( uint ) const;
    QString configPageFullName( uint ) const;
    QPixmap configPagePixmap( uint, int ) const;
    bool abDone;

  private slots:
    void slotCompleted();
    void applyEntity( AutoBookmarkEnt *e );
};

typedef Q3PtrList<AutoBookmarkEnt> ABEntityList;
typedef Q3PtrListIterator<AutoBookmarkEnt> ABEntityListIterator;

/* singleton to keep the data */
class ABGlobal
{
  public:
    ABGlobal();
    ~ABGlobal();

    static ABGlobal* self();

    ABEntityList* entities() { return m_ents; };
    void readConfig();
    void writeConfig();

  private:
    ABEntityList *m_ents;
    static ABGlobal *s_self;
};

class AutoBookmarkerConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT
  public:
    AutoBookmarkerConfigPage( QWidget *parent, const char *name );
    virtual ~AutoBookmarkerConfigPage() {}

    virtual void apply();
    virtual void reset();
    virtual void defaults();

  private slots:
    void slotNew();
    void slotDel();
    void slotEdit();

  private:
    class KListView *lvPatterns;
    class QPushButton *btnNew, *btnDel, *btnEdit;
    ABEntityList *m_ents;
};

class AutoBookmarkerEntEditor : public KDialogBase
{
  Q_OBJECT
  public:
    AutoBookmarkerEntEditor( QWidget *parent, AutoBookmarkEnt *e );
    ~AutoBookmarkerEntEditor(){}

    void apply();

  private slots:
  void showMTDlg();
    void slotPatternChanged( const QString& );
  private:
    class QLineEdit *lePattern, *leMimeTypes, *leFileMask;
    class QCheckBox *cbCS, *cbMM;
    AutoBookmarkEnt *e;
};

#endif //_AUTOBOOKMARKER_H_
