/***************************************************************************
                           plugin_katexmlcheck.cpp - checks XML files using xmllint
                           -------------------
	begin                : 2002-07-06
	copyright            : (C) 2002 by Daniel Naber
	email                : daniel.naber@t-online.de
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

/*
-fixme: show dock if "Validate XML" is selected (doesn't currently work when Kate
 was just started and the dockwidget isn't yet visible)
-fixme(?): doesn't correctly disappear when deactivated in config
*/

#include "plugin_katexmlcheck.h"
#include "plugin_katexmlcheck.moc"

#include <cassert>

#include <qfile.h>
#include <qinputdialog.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextstream.h>
#include <kactioncollection.h>
//Added by qt3to4:
#include <Q3CString>
#include <QApplication>

#include <kdefakes.h> // for setenv
#include <kaction.h>
#include <kcursor.h>
#include <kdebug.h>
#include <k3dockwidget.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katexmlcheckplugin, KGenericFactory<PluginKateXMLCheck>( "katexmlcheck" ) )

PluginKateXMLCheck::PluginKateXMLCheck( QObject* parent, const QStringList& )
	: Kate::Plugin ( (Kate::Application *)parent, "PluginKateXMLCheck" )
{
}


PluginKateXMLCheck::~PluginKateXMLCheck()
{
}

void PluginKateXMLCheck::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}
 
void PluginKateXMLCheck::loadViewConfig(KConfig* config, Kate::MainWindow*win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateXMLCheck::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateXMLCheck::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateXMLCheck::addView(Kate::MainWindow *win)
{
  //Kate::ToolViewManager *viewmanager = win;
	QWidget *dock = win->createToolView("kate_plugin_xmlcheck_ouputview", Kate::MainWindow::Bottom, SmallIcon("misc"), i18n("XML Checker Output"));

	PluginKateXMLCheckView *view = new PluginKateXMLCheckView (dock,win,"katexmlcheck_outputview");
	view->dock = dock;

	win->guiFactory()->addClient(view);
	view->win = win;

	m_views.append(view);
}


void PluginKateXMLCheck::removeView(Kate::MainWindow *win)
{
	for (uint z=0; z < m_views.count(); z++) {
		if (m_views.at(z)->win == win) {
			PluginKateXMLCheckView *view = m_views.at(z);
			m_views.remove (view);
			win->guiFactory()->removeClient (view);
			delete view->dock; // this will delete view, too
		}
	}
}


//---------------------------------
PluginKateXMLCheckView::PluginKateXMLCheckView(QWidget *parent,Kate::MainWindow *mainwin,const char* name)
	:Q3ListView(parent,name),KXMLGUIClient(),win(mainwin)
{
	m_tmp_file=0;
	m_proc=0;
	QAction *a = actionCollection()->addAction("xml_check");
	a->setText(i18n("Validate XML"));
	connect(a, SIGNAL(triggered()), this, SLOT(slotValidate()));
	// TODO?:
	//(void)  new KAction ( i18n("Indent XML"), KShortcut(), this,
	//	SLOT( slotIndent() ), actionCollection(), "xml_indent" );

	setComponentData(KComponentData("kate"));
	setXMLFile("plugins/katexmlcheck/ui.rc");


	setFocusPolicy(Qt::NoFocus);
	addColumn(i18n("#"), -1);
	addColumn(i18n("Line"), -1);
	setColumnAlignment(1, Qt::AlignRight);
	addColumn(i18n("Column"), -1);
	setColumnAlignment(2, Qt::AlignRight);
	addColumn(i18n("Message"), -1);
	setAllColumnsShowFocus(true);
	setResizeMode(Q3ListView::LastColumn);
	connect(this, SIGNAL(clicked(Q3ListViewItem *)), SLOT(slotClicked(Q3ListViewItem *)));

/* TODO?: invalidate the listview when document has changed
	Kate::View *kv = application()->activeMainWindow()->activeView();
	if( ! kv ) {
		kDebug() << "Warning: no Kate::View" << endl;
		return;
	}
	connect(kv, SIGNAL(modifiedChanged()), this, SLOT(slotUpdate()));
*/

	m_proc_stderr = "";
	m_proc = new KProcess();
	connect(m_proc, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcExited(KProcess*)));
	// we currently only want errors:
	//connect(m_proc, SIGNAL(receivedStdout(KProcess*,char*,int)),
	//	this, SLOT(receivedProcStdout(KProcess*, char*, int)));
	connect(m_proc, SIGNAL(receivedStderr(KProcess*,char*,int)),
		this, SLOT(slotReceivedProcStderr(KProcess*, char*, int)));

}

PluginKateXMLCheckView::~PluginKateXMLCheckView()
{
	delete m_proc;
	delete m_tmp_file;
}

void PluginKateXMLCheckView::slotReceivedProcStderr(KProcess *, char *result, int len)
{
	m_proc_stderr += QString::fromLocal8Bit( Q3CString(result, len+1) );
}

void PluginKateXMLCheckView::slotProcExited(KProcess*)
{
	// FIXME: doesn't work correct the first time:
	//if( m_dockwidget->isDockBackPossible() ) {
	//	m_dockwidget->dockBack();
//	}

	kDebug() << "slotProcExited()" << endl;
	//kDebug() << "output: " << endl << m_proc_stderr << endl << endl;
	QApplication::restoreOverrideCursor();
	delete m_tmp_file;
	m_tmp_file=0;
	clear();
	uint list_count = 0;
	uint err_count = 0;
	if( ! m_validating ) {
		// no i18n here, so we don't get an ugly English<->Non-english mixup:
		QString msg;
		if( m_dtdname.isEmpty() ) {
			msg = "No DOCTYPE found, will only check well-formedness.";
		} else {
			msg = "'" + m_dtdname + "' not found, will only check well-formedness.";
		}
		(void)new Q3ListViewItem(this, QString("1").rightJustified(4,' '), "", "", msg);
		list_count++;
	}
	if( ! m_proc_stderr.isEmpty() ) {
		QStringList lines = QStringList::split("\n", m_proc_stderr);
		Q3ListViewItem *item = 0;
		QString linenumber, msg;
		int line_count = 0;
		for(QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
			QString line = *it;
			line_count++;
			int semicolon_1 = line.find(':');
			int semicolon_2 = line.find(':', semicolon_1+1);
			int semicolon_3 = line.find(':', semicolon_2+2);
			int caret_pos = line.find('^');
			if( semicolon_1 != -1 && semicolon_2 != -1 && semicolon_3 != -1 ) {
				linenumber = line.mid(semicolon_1+1, semicolon_2-semicolon_1-1).trimmed();
				linenumber = linenumber.rightJustified(6, ' ');	// for sorting numbers
				msg = line.mid(semicolon_3+1, line.length()-semicolon_3-1).trimmed();
			} else if( caret_pos != -1 || line_count == lines.size() ) {
				// TODO: this fails if "^" occurs in the real text?!
				if( line_count == lines.size() && caret_pos == -1 ) {
					msg = msg+"\n"+line;
				}
				QString col = QString::number(caret_pos);
				if( col == "-1" ) {
					col = "";
				}
				err_count++;
				list_count++;
				item = new Q3ListViewItem(this, QString::number(list_count).rightJustified(4,' '), linenumber, col, msg);
				item->setMultiLinesEnabled(true);
			} else {
				msg = msg+"\n"+line;
			}
		}
		sort();	// TODO?: insert in right order
	}
	if( err_count == 0 ) {
		QString msg;
		if( m_validating ) {
			msg = "No errors found, document is valid.";	// no i18n here
		} else {
			msg = "No errors found, document is well-formed.";	// no i18n here
		}
		(void)new Q3ListViewItem(this, QString::number(list_count+1).rightJustified(4,' '), "", "", msg);
	}
}


void PluginKateXMLCheckView::slotClicked(Q3ListViewItem *item)
{
	kDebug() << "slotClicked" << endl;
	if( item ) {
		bool ok = true;
		uint line = item->text(1).toUInt(&ok);
		bool ok2 = true;
		uint column = item->text(2).toUInt(&ok);
		if( ok && ok2 ) {
			KTextEditor::View *kv = win->activeView();
			if( ! kv )
				return;

			kv->setCursorPosition(KTextEditor::Cursor (line-1, column));
		}
	}
}


void PluginKateXMLCheckView::slotUpdate()
{
	kDebug() << "slotUpdate() (not implemented yet)" << endl;
}


bool PluginKateXMLCheckView::slotValidate()
{
	kDebug() << "slotValidate()" << endl;

	win->showToolView (this);

	m_proc->clearArguments();
	m_proc_stderr = "";
	m_validating = false;
	m_dtdname = "";

	KTextEditor::View *kv = win->activeView();
	if( ! kv )
	  return false;

	m_tmp_file = new KTemporaryFile();
	if( !m_tmp_file->open() ) {
		kDebug() << "Error (slotValidate()): could not create '" << m_tmp_file->fileName() << "': " << m_tmp_file->errorString() << endl;
		KMessageBox::error(0, i18n("<b>Error:</b> Could not create "
			"temporary file '%1'.", m_tmp_file->fileName()));
		delete m_tmp_file;
		m_tmp_file=0L;
		return false;
	}
	QTextStream s ( m_tmp_file );
	s << kv->document()->text();
	s.flush();

	QString exe = KStandardDirs::findExe("xmllint");
	if( exe.isEmpty() ) {
		exe = KStandardDirs::locate("exe", "xmllint");
	}

	// use catalogs for KDE docbook:
	if( ! getenv("SGML_CATALOG_FILES") ) {
    	KComponentData ins("katexmlcheckplugin");
		QString catalogs;
		catalogs += ins.dirs()->findResource("data", "ksgmltools2/customization/catalog");
		catalogs += ":";
		catalogs += ins.dirs()->findResource("data", "ksgmltools2/docbook/xml-dtd-4.1.2/docbook.cat");
		kDebug() << "catalogs: " << catalogs << endl;
		setenv("SGML_CATALOG_FILES", QFile::encodeName( catalogs ).data(), 1);
	}
	//kDebug() << "**catalogs: " << getenv("SGML_CATALOG_FILES") << endl;

	*m_proc << exe << "--catalogs" << "--noout";

	// heuristic: assume that the doctype is in the first 10,000 bytes:
	QString text_start = kv->document()->text().left(10000);
	// remove comments before looking for doctype (as a doctype might be commented out
	// and needs to be ignored then):
	QRegExp re("<!--.*-->");
	re.setMinimal(true);
	text_start.replace(re, "");
	QRegExp re_doctype("<!DOCTYPE\\s+(.*)\\s+(?:PUBLIC\\s+[\"'].*[\"']\\s+[\"'](.*)[\"']|SYSTEM\\s+[\"'](.*)[\"'])", false);
	re_doctype.setMinimal(true);

	if( re_doctype.search(text_start) != -1 ) {
		QString dtdname;
		if( ! re_doctype.cap(2).isEmpty() ) {
			dtdname = re_doctype.cap(2);
		} else {
			dtdname = re_doctype.cap(3);
		}
		if( !dtdname.startsWith("http:") ) {		// todo: u_dtd.isLocalFile() doesn't work :-(
			// a local DTD is used
			m_validating = true;
			*m_proc << "--valid";
		} else {
			m_validating = true;
			*m_proc << "--valid";
		}
	} else if( text_start.find("<!DOCTYPE") != -1 ) {
		// DTD is inside the XML file
		m_validating = true;
		*m_proc << "--valid";
	}
	*m_proc << m_tmp_file->fileName();

	if( ! m_proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) ) {
		KMessageBox::error(0, i18n("<b>Error:</b> Failed to execute xmllint. Please make "
			"sure that xmllint is installed. It is part of libxml2."));
		return false;
	}
	QApplication::setOverrideCursor(KCursor::waitCursor());
	return true;
}
