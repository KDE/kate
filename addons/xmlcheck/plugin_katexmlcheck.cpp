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

// TODO:
// Cleanup unneeded headers
// Find resources and translate i18n messages
// all translations were deleted in https://websvn.kde.org/?limit_changes=0&view=revision&revision=1433517
// What to do with catalogs? What is it for?
// Implement hot key shortcut to do xml validation
// Remove copyright above due to author orphaned this plugin?
// Possibility to check only well-formdness without validation
// Hide output in dock when switching to another tab
// Make ability to validate against xml schema and then edit docbook
// Should del space in [km] strang in katexmlcheck.desktop?
// Which variant should I choose? QUrl.adjusted(rm filename).path() or QUrl.toString(rm filename|rm schema)
// What about replace xmllint xmlstarlet or something?
// Maybe use QXmlReader to take dtds and xsds?

#include "plugin_katexmlcheck.h"

#include <KActionCollection>
#include <QApplication>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "hostprocess.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QAction>
#include <QTemporaryFile>

#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

#include <ktexteditor/editor.h>

#include <QRegularExpression>
#include <kxmlguifactory.h>

K_PLUGIN_FACTORY_WITH_JSON(PluginKateXMLCheckFactory, "katexmlcheck.json", registerPlugin<PluginKateXMLCheck>();)

PluginKateXMLCheck::PluginKateXMLCheck(QObject *const parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
}

PluginKateXMLCheck::~PluginKateXMLCheck()
{
}

QObject *PluginKateXMLCheck::createView(KTextEditor::MainWindow *mainWindow)
{
    return new PluginKateXMLCheckView(this, mainWindow);
}

//---------------------------------
PluginKateXMLCheckView::PluginKateXMLCheckView(KTextEditor::Plugin *, KTextEditor::MainWindow *mainwin)
    : QObject(mainwin)
    , m_mainWindow(mainwin)
    , m_provider(mainwin, this)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katexmlcheck"), i18n("XML Check")); // where i18n resources?
    setXMLFile(QStringLiteral("ui.rc"));

    m_tmp_file = nullptr;
    QAction *a = actionCollection()->addAction(QStringLiteral("xml_check"));
    a->setText(i18n("Validate XML"));
    connect(a, &QAction::triggered, this, &PluginKateXMLCheckView::slotValidate);
    // TODO?:
    //(void)  new KAction ( i18n("Indent XML"), KShortcut(), this,
    //	SLOT(slotIndent()), actionCollection(), "xml_indent" );

    connect(&m_proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &PluginKateXMLCheckView::slotProcExited);
    // we currently only want errors:
    m_proc.setProcessChannelMode(QProcess::SeparateChannels);
    // m_proc.setProcessChannelMode(QProcess::ForwardedChannels); // For Debugging. Do not use this.

    mainwin->guiFactory()->addClient(this);
}

PluginKateXMLCheckView::~PluginKateXMLCheckView()
{
    m_mainWindow->guiFactory()->removeClient(this);
    delete m_tmp_file;
}

void PluginKateXMLCheckView::slotProcExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    // FIXME: doesn't work correct the first time:
    // if( m_dockwidget->isDockBackPossible() ) {
    //	m_dockwidget->dockBack();
    //	}

    if (exitStatus != QProcess::NormalExit) {
        Utils::showMessage(i18n("Validate process crashed"), {}, i18n("XMLCheck"), MessageType::Error);
        return;
    }

    qDebug("slotProcExited()");
    QApplication::restoreOverrideCursor();
    delete m_tmp_file;
    QString proc_stderr = QString::fromLocal8Bit(m_proc.readAllStandardError());
    m_tmp_file = nullptr;
    uint err_count = 0;
    if (!m_validating) {
        // no i18n here, so we don't get an ugly English<->Non-english mixup:
        QString msg;
        if (m_dtdname.isEmpty()) {
            msg = i18n("No DOCTYPE found, will only check well-formedness.");
        } else {
            msg = i18nc("%1 refers to the XML DTD", "'%1' not found, will only check well-formedness.", m_dtdname);
        }
        Utils::showMessage(msg, {}, i18n("XMLCheck"), MessageType::Warning);
    }
    if (!proc_stderr.isEmpty()) {
        QList<Diagnostic> diags;
        QStringList lines = proc_stderr.split(u'\n', Qt::SkipEmptyParts);
        QString linenumber, msg;
        int line_count = 0;
        for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
            const QString &line = *it;
            line_count++;
            int semicolon_1 = line.indexOf(u':');
            int semicolon_2 = line.indexOf(u':', semicolon_1 + 1);
            int semicolon_3 = line.indexOf(u':', semicolon_2 + 2);
            int caret_pos = line.indexOf(u'^');
            if (semicolon_1 != -1 && semicolon_2 != -1 && semicolon_3 != -1) {
                linenumber = line.mid(semicolon_1 + 1, semicolon_2 - semicolon_1 - 1).trimmed();
                linenumber = linenumber.rightJustified(6, u' '); // for sorting numbers
                msg = line.mid(semicolon_3 + 1, line.length() - semicolon_3 - 1).trimmed();
            } else if (caret_pos != -1 || line_count == lines.size()) {
                // TODO: this fails if "^" occurs in the real text?!
                if (line_count == lines.size() && caret_pos == -1) {
                    msg = msg + u'\n' + line;
                }
                QString col = QString::number(caret_pos);
                if (col == QLatin1String("-1")) {
                    col = QLatin1String("");
                }
                err_count++;
                // Diag item here
                Diagnostic d;
                int ln = linenumber.toInt() - 1;
                ln = ln >= 0 ? ln : 0;
                int cl = col.toInt() - 1;
                cl = cl >= 0 ? cl : 0;
                d.range = {ln, cl, ln, cl};
                d.message = msg;
                d.source = QStringLiteral("xmllint");
                d.severity = DiagnosticSeverity::Warning;
                diags << d;
            } else {
                msg = msg + u'\n' + line;
            }
        }
        if (!diags.empty()) {
            if (auto v = m_mainWindow->activeView()) {
                FileDiagnostics fd;
                fd.uri = v->document()->url();
                fd.diagnostics = diags;
                Q_EMIT m_provider.diagnosticsAdded(fd);
                m_provider.showDiagnosticsView();
            }
        }
    }
    if (err_count == 0) {
        QString msg;
        if (m_validating) {
            msg = QStringLiteral("No errors found, document is valid."); // no i18n here
        } else {
            msg = QStringLiteral("No errors found, document is well-formed."); // no i18n here
        }
        Utils::showMessage(msg, {}, i18n("XMLCheck"), MessageType::Info);
    }
}

void PluginKateXMLCheckView::slotUpdate()
{
    qDebug("slotUpdate() (not implemented yet)");
}

bool PluginKateXMLCheckView::slotValidate()
{
    qDebug("slotValidate()");

    m_validating = false;
    m_dtdname = QLatin1String("");

    KTextEditor::View *kv = m_mainWindow->activeView();
    if (!kv) {
        return false;
    }
    delete m_tmp_file;
    m_tmp_file = new QTemporaryFile();
    if (!m_tmp_file->open()) {
        qDebug("Error (slotValidate()): could not create '%ls': %ls", qUtf16Printable(m_tmp_file->fileName()), qUtf16Printable(m_tmp_file->errorString()));
        const QString msg = i18n("<b>Error:</b> Could not create temporary file '%1'.", m_tmp_file->fileName());
        Utils::showMessage(msg, {}, i18n("XMLCheck"), MessageType::Error, m_mainWindow);
        delete m_tmp_file;
        m_tmp_file = nullptr;
        return false;
    }

    QTextStream s(m_tmp_file);
    s << kv->document()->text();
    s.flush();

    // ensure we only execute xmllint from PATH or application package
    static const auto executableName = QStringLiteral("xmllint");
    QString exe = safeExecutableName(executableName);
    if (exe.isEmpty()) {
        exe = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, executableName);
    }
    if (exe.isEmpty()) {
        const QString msg = i18n(
            "<b>Error:</b> Failed to find xmllint. Please make "
            "sure that xmllint is installed. It is part of libxml2.");
        Utils::showMessage(msg, {}, i18n("XMLCheck"), MessageType::Error, m_mainWindow);
        return false;
    }

    // qDebug() << "exe=" <<exe;
    // 	// use catalogs for KDE docbook:
    // 	if( ! getenv("XML_CATALOG_FILES") ) {
    // 		KComponentData ins("katexmlcheckplugin");
    // 		QString catalogs;
    // 		catalogs += ins.dirs()->findResource("data", "ksgmltools2/customization/catalog.xml");
    // 		qDebug() << "catalogs: " << catalogs;
    // 		setenv("XML_CATALOG_FILES", QFile::encodeName( catalogs ).data(), 1);
    // 	}
    // qDebug() << "**catalogs: " << getenv("XML_CATALOG_FILES");
    QStringList args;
    args << QStringLiteral("--noout");

    // tell xmllint the working path of the document's file, if possible.
    // otherwise it will not find relative DTDs

    // I should give path to location of file, but remove filename
    // I can make QUrl.adjusted(rm filename).path()
    // or QUrl.toString(rm filename|rm schema)
    // Result is the same. Which variant should I choose?
    // QString path = kv->document()->url().adjusted(QUrl::RemoveFilename).path();
    // xmllint uses space- or colon-separated path option, so spaces should be encoded to %20. It is done with EncodeSpaces.

    // Now what about colons in file names or paths?
    // This way xmllint works normally:
    // xmllint --noout --path "/home/user/my/with:colon/" --valid "/home/user/my/with:colon/demo-1.xml"
    // but because this plugin makes temp file path to file is another and this way xmllint refuses to find dtd:
    // xmllint --noout --path "/home/user/my/with:colon/" --valid "/tmp/kate.X23725"
    // As workaround we can encode ':' with %3A
    QString path = kv->document()->url().toString(QUrl::RemoveFilename | QUrl::PreferLocalFile | QUrl::EncodeSpaces);
    path.replace(u':', QLatin1String("%3A"));
    // because of such inconvenience with xmllint and paths, maybe switch to xmlstarlet?

    qDebug("path=%ls", qUtf16Printable(path));

    if (!path.isEmpty()) {
        args << QStringLiteral("--path") << path;
    }

    // heuristic: assume that the doctype is in the first 10,000 bytes:
    QString text_start = kv->document()->text().left(10000);
    // remove comments before looking for doctype (as a doctype might be commented out
    // and needs to be ignored then):
    static const QRegularExpression re(QStringLiteral("<!--.*-->"), QRegularExpression::InvertedGreedinessOption);
    text_start.remove(re);
    static const QRegularExpression re_doctype(QStringLiteral("<!DOCTYPE\\s+(.*)\\s+(?:PUBLIC\\s+[\"'].*[\"']\\s+[\"'](.*)[\"']|SYSTEM\\s+[\"'](.*)[\"'])"),
                                               QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);

    if (QRegularExpressionMatch match = re_doctype.match(text_start); match.hasMatch()) {
        QString dtdname;
        if (!match.captured(2).isEmpty()) {
            dtdname = match.captured(2);
        } else {
            dtdname = match.captured(2);
        }
        if (!dtdname.startsWith(QLatin1String("http:"))) { // todo: u_dtd.isLocalFile() doesn't work :-(
            // a local DTD is used
            m_validating = true;
            args << QStringLiteral("--valid");
        } else {
            m_validating = true;
            args << QStringLiteral("--valid");
        }
    } else if (text_start.indexOf(QLatin1String("<!DOCTYPE")) != -1) {
        // DTD is inside the XML file
        m_validating = true;
        args << QStringLiteral("--valid");
    }
    args << m_tmp_file->fileName();
    qDebug("m_tmp_file->fileName()=%ls", qUtf16Printable(m_tmp_file->fileName()));

    startHostProcess(m_proc, exe, args);
    qDebug("m_proc.program():%ls", qUtf16Printable(m_proc.program())); // I want to see parameters
    qDebug("args=%ls", qUtf16Printable(args.join(u'\n')));
    qDebug("exit code: %d", m_proc.exitCode());
    if (!m_proc.waitForStarted(-1)) {
        const QString msg = i18n(
            "<b>Error:</b> Failed to execute xmllint. Please make "
            "sure that xmllint is installed. It is part of libxml2.");
        Utils::showMessage(msg, {}, i18n("XMLCheck"), MessageType::Error, m_mainWindow);
        return false;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    return true;
}

#include "plugin_katexmlcheck.moc"
