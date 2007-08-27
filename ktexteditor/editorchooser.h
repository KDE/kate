/* This file is part of the KDE libraries
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

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
#ifndef _EDITOR_CHOOSER_H_
#define _EDITOR_CHOOSER_H_

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include <QtGui/QWidget>

class KConfig;
class QString;

namespace KTextEditor
{

/**
 * \brief Editor Component Chooser.
 *
 * Topics:
 *  - \ref chooser_intro
 *  - \ref chooser_gui
 *  - \ref chooser_editor
 *
 * \section chooser_intro Introduction
 *
 * The EditorChooser is responsible for two different tasks: It provides
 *  - a GUI, with which the user can choose the preferred editor part
 *  - a static accessor, with which the current selected editor part can be
 *    obtained.
 *
 * \section chooser_gui The GUI Editor Chooser
 * The EditorChooser is a simple widget with a group box containing an
 * information label and a combo box which lists all available KTextEditor
 * implementations. To give the user the possibility to choose a text editor
 * implementation, create an instance of this class and put it into the GUI:
 * \code
 * KTextEditor::EditorChooser* ec = new KTextEditor::EditorChooser(parent);
 * // read the settings from the application's KConfig object
 * ec->readAppSetting();
 * // optionally connect the signal changed()
 * // plug the widget into a layout
 * layout->addWidget(ec);
 * \endcode
 * Later, for example when the user clicks the Apply-button:
 * \code
 * // save the user's choice
 * ec->writeAppSetting();
 * \endcode
 * After this, the static accessor editor() will return the new editor part
 * object. Now, either the application has to be restarted, or you need code
 * that closes all current documents so that you can safely switch and use the
 * new editor part. Restarting is probably much easier.
 *
 * \note If you do not put the EditorChooser into the GUI, the default editor
 *       component will be used. The default editor is configurable in KDE's
 *       control center in
 *       "KDE Components > Component Chooser > Embedded Text Editor".
 *
 * \section chooser_editor Accessing the Editor Part
 * The call of editor() will return the currently used editor part, either the
 * KDE default or the one configured with the EditorChooser's GUI
 * (see \ref chooser_gui). Example:
 * \code
 * KTextEditor::Editor* editor = KTextEditor::EditorChooser::editor();
 * \endcode
 *
 * \see KTextEditor::Editor
 * \author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT EditorChooser: public QWidget
{
  friend class PrivateEditorChooser;

  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create an editor chooser widget.
     * \param parent the parent widget
     */
    EditorChooser(QWidget *parent=0);
    /**
     * Destructor.
     */
    virtual ~EditorChooser();

   /* void writeSysDefault();*/

    /**
     * Read the configuration from the application's config file. The group
     * is handeled internally (it is called "KTEXTEDITOR:", but it is possible
     * to add a string to extend the group name with \p postfix.
     * \param postfix config group postfix string
     * \see writeAppSetting()
     */
    void readAppSetting(const QString& postfix=QString());
    /**
     * Write the configuration to the application's config file.
     * \param postfix config group postfix string
     * \see readAppSetting()
     */
    void writeAppSetting(const QString& postfix=QString());

    /**
     * Static accessor to get the Editor instance of the currently used
     * KTextEditor component.
     *
     * That Editor instance can be qobject-cast to specific extensions.
     * If the result of the cast is not NULL, that extension is supported:
     *
     * \param postfix config group postfix string
     * \param fallBackToKatePart if \e true, the returned Editor component
     *        will be a katepart if no other implementation can be found
     * \return Editor controller or NULL, if no editor component could be
     *        found
     */
    static KTextEditor::Editor *editor (const QString& postfix=QString(), bool fallBackToKatePart = true);

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the selected item in the combo box
     * changed.
     */
    void changed();
  private:
    class PrivateEditorChooser *d;
};

/*
class EditorChooserBackEnd: public ComponentChooserPlugin {

Q_OBJECT
public:
	EditorChooserBackEnd(QObject *parent=0, const char *name=0);
	virtual ~EditorChooserBackEnd();

	virtual QWidget *widget(QWidget *);
	virtual const QStringList &choices();
	virtual void saveSettings();

	void readAppSetting(KConfig *cfg,const QString& postfix);
	void writeAppSetting(KConfig *cfg,const QString& postfix);

public Q_SLOTS:
	virtual void madeChoice(int pos,const QString &choice);

};
*/

}
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
