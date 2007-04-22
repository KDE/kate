/* This file is part of the KDE project
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_CONFIGPAGE_H
#define KDELIBS_KTEXTEDITOR_CONFIGPAGE_H

#include <ktexteditor/ktexteditor_export.h>

#include <QtGui/QWidget>

namespace KTextEditor
{

/**
 * \brief Config page interface for the Editor.
 *
 * \section configpage_intro Introduction
 *
 * The class ConfigPage represents a config page of the KTextEditor::Editor.
 * The Editor's config pages are usually embedded into a dialog that shows
 * buttons like \e Defaults, \e Reset and \e Apply. If one of the buttons is
 * clicked and the condig page sent the signal changed() beforehand the
 * Editor will call the corresponding slot, either defaults(), reset() or
 * apply().
 *
 * \see KTextEditor::Editor
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT ConfigPage : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new config page with \p parent.
     * \param parent parent widget
     */
    ConfigPage ( QWidget *parent );
    /**
     * Virtual destructor.
     */
    virtual ~ConfigPage ();

  public Q_SLOTS:
    /**
     * This slot is called whenever the button \e Apply or \e OK was clicked.
     * Apply the changed settings made in the config page now.
     */
    virtual void apply () = 0;

    /**
     * This slot is called whenever the button \e Reset was clicked.
     * Reset the config page settings to the initial state.
     */
    virtual void reset () = 0;

    /**
     * Sets default options
     * This slot is called whenever the button \e Defaults was clicked.
     * Set the config page settings to the default values.
     */
    virtual void defaults () = 0;

  Q_SIGNALS:
    /**
     * Emit this signal whenever a config option changed.
     */
    void changed();

  private:
    class ConfigPagePrivate* const d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
