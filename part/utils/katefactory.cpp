/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateglobal.h"
#include "katedocument.h"

#include <ktexteditor/factory.h>

#include <kpluginfactory.h>

/**
 * wrapper factory to be sure nobody external deletes our kateglobal object
 * each instance will just increment the reference counter of our internal
 * super private global instance ;)
 */
class KateFactory : public KTextEditor::Factory
{
  Q_OBJECT
  
  Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "")
  
  Q_INTERFACES(KPluginFactory)
  
  public:
    /**
     * This constructor creates a factory for a plugin with the given \p componentName.
     *
     * \param componentName the component name of the plugin
     * \param parent a parent object
     */
    explicit KateFactory (const char *componentName = 0, QObject *parent = 0)
      : KTextEditor::Factory (componentName, parent)
    {
      KateGlobal::incRef ();
    }

    /**
     * destructor, release editor
     */
    virtual ~KateFactory ()
    {
      KateGlobal::decRef ();
    }

    KTextEditor::Editor *editor () { return KateGlobal::self(); }

    
    /**
     * This function is called when the factory asked to create an Object.
     *
     * You may reimplement it to provide a very flexible factory. This is especially useful to
     * provide generic factories for plugins implemeted using a scripting language.
     *
     * \param iface The staticMetaObject::className() string identifying the plugin interface that
     * was requested. E.g. for KCModule plugins this string will be "KCModule".
     * \param parentWidget Only used if the requested plugin is a KPart.
     * \param parent The parent object for the plugin object.
     * \param args A plugin specific list of arbitrary arguments.
     * \param keyword A string that uniquely identifies the plugin. If a KService is used this
     * keyword is read from the X-KDE-PluginKeyword entry in the .desktop file.
     */
    virtual QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword)
    {
      QByteArray classname( iface );

      // default to the kparts::* behavior of having one single widget() if the user don't requested a pure document
      bool bWantSingleView = ( classname != "KTextEditor::Document" );

      // does user want browserview?
      bool bWantBrowserView = ( classname == "Browser/View" );

      // should we be readonly?
      bool bWantReadOnly = (bWantBrowserView || ( classname == "KParts::ReadOnlyPart" ));

      KParts::ReadWritePart *part = new KateDocument (bWantSingleView, bWantBrowserView, bWantReadOnly, parentWidget, parent);
      part->setReadWrite( !bWantReadOnly );

      return part;
    }
};

#include "katefactory.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
