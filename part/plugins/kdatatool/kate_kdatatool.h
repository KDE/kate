#ifndef _KATE_KDATATOOL_
#define _KATE_KDATATOOL_

#include <ktexteditor/plugin.h>
#include <qstringlist.h>
#include <kxmlguiclient.h>
#include <qguardedptr.h>

class KActionMenu;
class KDataToolInfo;

namespace KTextEditor
{

class View;

class KDataToolPlugin : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface
{
  Q_OBJECT

  public:
          KDataToolPlugin( QObject *parent = 0, const char* name = 0, const QStringList &args = QStringList() );
          virtual ~KDataToolPlugin();

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view){;}

//  private:
//    QPtrList<class ISearchPluginView> m_views;
};


class KDataToolPluginView : public QObject, public KXMLGUIClient
{
        Q_OBJECT

public:
        KDataToolPluginView( KTextEditor::View *view );
        virtual ~KDataToolPluginView();

  	void setView( KTextEditor::View* view ){;}
private:
	View *m_view;
	bool m_singleWord;
	QString m_wordUnderCursor;
	QPtrList<KAction> m_actionList;
	QGuardedPtr<KActionMenu> m_menu;
protected slots:
	void aboutToShow();
	void slotToolActivated( const KDataToolInfo &datatoolinfo, const QString &string );
};

};

#endif
