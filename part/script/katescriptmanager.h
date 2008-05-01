
/// This file is part of the KDE libraries
/// Copyright(C) 2008 Paul Giannaros <paul@giannaros.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License as published by the Free Software Foundation; either
/// version 2 of the License, or (at your option) version 3.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#ifndef KATE_SCRIPT_MANAGER_H
#define KATE_SCRIPT_MANAGER_H

#include <ktexteditor/commandinterface.h>
#include <ktexteditor/cursor.h>

#include <kdebug.h>

#include <QVector>

#include "katescript.h"
#include "kateindentscript.h"

class QString;


/**
 * Manage the scripts on disks -- find them and query them.
 * Provides access to loaded scripts too.
 */
class KateScriptManager : public KTextEditor::Command
{

  public:
    KateScriptManager();
    ~KateScriptManager();
    
    /** Get a vector containing all of the scripts that the manager knows about */
    const QVector<KateScript*> &scripts() { return m_scripts; }
    
    /**
     * Get an indentation script for the given language -- if there is more than
     * one result, this will return the script with the highest priority. If
     * both have the same priority, an arbitrary script will be returned.
     * If no scripts are found, 0 is returned.
     */
    KateIndentScript *indenter(const QString &language);
    
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec(KTextEditor::View *view, const QString &cmd, QString &errorMsg);

    /**
     * get help for a command
     * @param view view to use
     * @param cmd cmd name
     * @param msg help message
     * @return help available or not
     */
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg);

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds();
    
    // Helper methods

    /**
     * Find all of the scripts matching the wildcard \p directory. The resource file
     * with the name \p resourceFile is used for caching. If \p force is true, then the
     * cache will not be used. This populates the internal lists of scripts.
     * This is automatically called by init so you shouldn't call it yourself unless
     * you want to refresh the collected list.
     */
    void collect(const QString& resourceFile,
                 const QString& directory,
                 bool force = false);
    
    /**
     * Extract the meta data from the script at \p url and put in \p pairs.
     * Returns true if metadata was found and extracted successfuly, false otherwise.
    */
    static bool parseMetaInformation(const QString& url, QHash<QString, QString> &pairs);

  public:
    KateIndentScript *indentationScript (const QString &scriptname) { return m_indentationScripts.value(scriptname); }

    int indentationScripts () { return m_indentationScriptsList.size(); }
    KateIndentScript *indentationScriptByIndex (int index) { return m_indentationScriptsList[index]; }

  private:
    /** Vector containing all of the scripts that we know about */
    QVector<KateScript*> m_scripts;
    /** Map of language to indent scripts */
    QHash<QString, QVector<KateIndentScript*> > m_languageToIndenters;

    /**
     * hash of all existing indenter scripts
     * hashes basename -> script
     */
    QHash<QString, KateIndentScript*> m_indentationScripts;

    /**
     * list of all indentation scripts
     */
    QList<KateIndentScript*> m_indentationScriptsList;
};



#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
