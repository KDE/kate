/***************************************************************************
  A KTextEditor (Kate Part) plugin for speaking text.

  Copyright:
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  (C) 2009 by Laurent Montel <montel@kde.org>

  Original Author: Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// KateKttsdPlugin includes.
#include "katekttsd.h"
#include "katekttsd.moc"
#include <ktexteditor/document.h>
// Qt includes.
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>

// KDE includes.
#include <kmessagebox.h>
#include <QAction>
#include <klocalizedstring.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>
#include <KActionCollection>
#include <KAboutData>
#include <kate/mainwindow.h>

K_PLUGIN_FACTORY(KateKttsdFactory, registerPlugin<KateKttsdPlugin>();)
K_EXPORT_PLUGIN(KateKttsdFactory(KAboutData("kate_kttsd","kate_kttsd",ki18n("Jovie Text-to-Speech Plugin"), "0.1", ki18n("Jovie Text-to-Speech Plugin"), KAboutData::License_LGPL_V2)) )

KateKttsdPlugin::KateKttsdPlugin(QObject* parent, const QList<QVariant>&)
    : Kate::Plugin ((Kate::Application*)parent)
{
}

Kate::PluginView *KateKttsdPlugin::createView (Kate::MainWindow *mainWindow)
{
    return new KateKttsdPluginView(mainWindow);
}

KateKttsdPluginView::KateKttsdPluginView( Kate::MainWindow *mw )
    : Kate::PluginView (mw),
    Kate::XMLGUIClient(KateKttsdFactory::componentData())
{
    KGlobal::locale()->insertCatalog("kttsd");
    KAction *a = actionCollection()->addAction("tools_kttsd");
    a->setText(i18n("Speak Text"));
    a->setIcon(KIcon("preferences-desktop-text-to-speech"));
    connect( a, SIGNAL(triggered(bool)), this, SLOT(slotReadOut()) );

    mainWindow()->guiFactory()->addClient(this);
}

KateKttsdPluginView::~KateKttsdPluginView()
{
    mainWindow()->guiFactory()->removeClient( this );
}


void KateKttsdPluginView::slotReadOut()
{
    KTextEditor::View *v = mainWindow()->activeView();
    if ( !v )
        return;
    KTextEditor::Document *doc = v->document();
    QString text;
    if ( v->selection() )
    {
        text = v->selectionText();
    }
    else
        text = doc->text();
    if ( text.isEmpty() )
        return;

    // If KTTSD not running, start it.
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
    {
        QString error;
        if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
        {
            KMessageBox::error(0, i18n( "Starting Jovie Text-to-Speech Service Failed"), error );
            return;
        }
    }

    QDBusInterface kttsd( "org.kde.kttsd", "/KSpeech", "org.kde.KSpeech" );

    QDBusReply<int> reply = kttsd.call("say", text,0);
    if ( !reply.isValid())
        KMessageBox::error( 0, i18n( "D-Bus Call Failed" ),
                              i18n( "The D-Bus call say failed." ));
}

