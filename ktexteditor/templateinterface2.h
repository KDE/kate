/* This file is part of the KDE libraries
   Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

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
*/

#ifndef KDELIBS_KTEXTEDITOR_TEMPLATEINTERFACE2_H
#define KDELIBS_KTEXTEDITOR_TEMPLATEINTERFACE2_H

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtGui/QImage>

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/cursor.h>
#include <ktexteditor/templateinterface.h>

namespace KTextEditor
{

class Cursor;

class KTEXTEDITOR_EXPORT TemplateScript
{
  public:
    virtual ~TemplateScript();
};

/**
 * @since 4.5
 * This is an interface for inserting template strings with user editable
 * fields into a document and support for scripts. Fold back into base Interface in KDE 5
 * \ingroup kte_group_view_extensions
 */
class KTEXTEDITOR_EXPORT TemplateInterface2: public TemplateInterface
{
  public:
    TemplateInterface2();
    virtual ~TemplateInterface2();

  public:

    /**
     * See the function  description in TemplateInterface, this should be folded into the base Interface in KDE 5
     * @param templateScript pointer to TemplateScript created by TemplateScriptRegistrar::registerTemplateScript
     */
    bool insertTemplateText ( const Cursor &insertPosition,
                              const QString &templateString,
                              const QMap<QString,QString> &initialValues,
                              TemplateScript* templateScript);


protected:
    /**
     * You must implement this, it is called by insertTemplateText, after all
     * default values are inserted. If you are implementing this interface,
     * this method should work as described in the documentation for
     * insertTemplateText above.
     * \return true if any text was inserted.
     */
    virtual bool insertTemplateTextImplementation ( const Cursor &insertPosition,
                                                    const QString &templateString,
                                                    const QMap<QString,QString> &initialValues,
                                                    TemplateScript* templateScript) = 0;


    virtual bool insertTemplateTextImplementation ( const Cursor &insertPosition,
                                                    const QString &templateString,
                                                    const QMap<QString,QString> &initialValues) = 0;

  private:
    class TemplateInterfacePrivate2* const d;
};

/// This is an extension for KTextEditor::Editor
/// @since 4.5
class KTEXTEDITOR_EXPORT TemplateScriptRegistrar {

public:
    TemplateScriptRegistrar();
    virtual ~TemplateScriptRegistrar();

    /**
     * This registeres the script, which is contained in \param script.
     * \return the template script pointer, returns an empty QString on error
     * The implementation has to register the script for all views and all documents,
     * == globally
     * If owner is destructed, all scripts owned by it are automatically freed.
     * Scripts have to be self contained.
     * Depending on the underlying editor, there might be some global functions,
     * perhaps there will be a specifiction for a common functionset later on, but not
     * yet.
     */
    virtual TemplateScript* registerTemplateScript(QObject *owner, const QString& script) = 0;

    /**
     * This frees the template script which is identified by the token
     */
    virtual void unregisterTemplateScript(TemplateScript* templateScript) = 0;

};

}

Q_DECLARE_INTERFACE(KTextEditor::TemplateInterface2,
"org.kde.KTextEditor.TemplateInterface2")

Q_DECLARE_INTERFACE(KTextEditor::TemplateScriptRegistrar,
"org.kde.KTextEditor.TemplateScriptRegistrar")



#endif
