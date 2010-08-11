/* Description : Kate CTags plugin
 * 
 * Copyright (C) 2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kate_ctags_plugin.h"

#include <QFileInfo>
#include <KFileDialog>
#include <QCheckBox>

#include <kgenericfactory.h>
#include <kmenu.h>
#include <kactioncollection.h>
#include <kstringhandler.h>
#include <kmessagebox.h>

#define DEFAULT_CTAGS_CMD "ctags -R --c++-types=+px --excmd=pattern --exclude=Makefile --exclude=."

K_EXPORT_COMPONENT_FACTORY(katectagsplugin, KGenericFactory<KateCTagsPlugin>("kate-ctags-plugin"))

/******************************************************************/
KateCTagsPlugin::KateCTagsPlugin(QObject* parent, const QStringList&):
Kate::Plugin ((Kate::Application*)parent), m_view(0)
{
}

/******************************************************************/
Kate::PluginView *KateCTagsPlugin::createView(Kate::MainWindow *mainWindow)
{
    m_view = new KateCTagsView(mainWindow);
    return m_view;
}


/******************************************************************/
Kate::PluginConfigPage *KateCTagsPlugin::configPage (uint number, QWidget *parent, const char *)
{
  if (number != 0) return 0;
  return new KateCTagsConfigPage(parent, this);
}

/******************************************************************/
QString KateCTagsPlugin::configPageName (uint number) const
{
    if (number != 0) return QString();
    return i18n("CTags");
}

/******************************************************************/
QString KateCTagsPlugin::configPageFullName (uint number) const
{
    if (number != 0) return QString();
    return i18n("CTags Settings");
}

/******************************************************************/
KIcon KateCTagsPlugin::configPageIcon (uint number) const
{
    if (number != 0) return KIcon();
    return KIcon("text-x-csrc");
}

/******************************************************************/
void KateCTagsPlugin::readConfig()
{
    m_view->readConfig();
}

/******************************************************************/
KateCTagsConfigPage::KateCTagsConfigPage( QWidget* parent, KateCTagsPlugin *plugin )
: Kate::PluginConfigPage( parent )
, m_plugin( plugin )
{
    QVBoxLayout *lo = new QVBoxLayout( this );
    lo->setSpacing( KDialog::spacingHint() );
    
    m_cbAutoSyncronize = new QCheckBox( i18n("&Automatically synchronize the terminal with the current document when possible"), this );
    lo->addWidget( m_cbAutoSyncronize );
    m_cbSetEditor = new QCheckBox( i18n("Set &EDITOR environment variable to 'kate -b'"), this );
    lo->addWidget( m_cbSetEditor );
    QLabel *tmp = new QLabel(this);
    tmp->setText(i18n("Important: The document has to be closed to make the console application continue"));
    lo->addWidget(tmp);
    reset();
    lo->addStretch();
    connect( m_cbAutoSyncronize, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
    connect( m_cbSetEditor, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
}

/******************************************************************/
void KateCTagsConfigPage::apply()
{
    KConfigGroup config(KGlobal::config(), "CTags");
    config.writeEntry("AutoSyncronize", m_cbAutoSyncronize->isChecked());
    config.writeEntry("SetEditor", m_cbSetEditor->isChecked());
    config.sync();
    m_plugin->readConfig();
}

/******************************************************************/
void KateCTagsConfigPage::reset()
{
    KConfigGroup config(KGlobal::config(), "CTags");
    m_cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
    m_cbSetEditor->setChecked(config.readEntry("SetEditor", false));
}



