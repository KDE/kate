/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __kate_filetype_h__
#define __kate_filetype_h__

#include <qstringlist.h>
#include <q3ptrlist.h>
#include <q3popupmenu.h> // for QPtrList<QPopupMenu>, compile with gcc 3.4
#include <qpointer.h>

#include "katedialogs.h"

class KateDocument;

class KateFileType
{
  public:
    int number;
    QString name;
    QString section;
    QStringList wildcards;
    QStringList mimetypes;
    int priority;
    QString varLine;
};

class KateFileTypeManager
{
  public:
    KateFileTypeManager ();
    ~KateFileTypeManager ();

    /**
     * File Type Config changed, update all docs (which will take care of views/renderers)
     */
    void update ();

    void save (const QList<KateFileType>& v);

    /**
     * get the right fileType for the given document
     * -1 if none found !
     */
    int fileType (KateDocument *doc);

    bool isValidType(int number) const;

    /**
     * Don't store the pointer somewhere longer times, won't be valid after the next update()
     */
    const KateFileType& fileType (int number) const;

    /**
     * Don't modify
     */
    const QList<KateFileType>& list() const { return m_types; }

  private:
    int wildcardsFind (const QString &fileName);

  private:
    QList<KateFileType> m_types;
};

class KateFileTypeConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateFileTypeConfigTab( QWidget *parent );

  public slots:
    void apply();
    void reload();
    void reset();
    void defaults();

  private slots:
    void update ();
    void deleteType ();
    void newType ();
    void typeChanged (int type);
    void showMTDlg();
    void save ();

  private:
    class QGroupBox *gbProps;
    class QPushButton *btndel;
    class QComboBox *typeCombo;
    class QLineEdit *wildcards;
    class QLineEdit *mimetypes;
    class KIntNumInput *priority;
    class QLineEdit *name;
    class QLineEdit *section;
    class QLineEdit *varLine;

    QList<KateFileType> m_types;
    int m_lastType;
};

class KateViewFileTypeAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewFileTypeAction(const QString& text, KActionCollection* parent = 0, const char* name = 0)
       : KActionMenu(text, parent, name) { init(); };

    ~KateViewFileTypeAction();

    void updateMenu (KTextEditor::Document *doc);

  private:
    void init();

    QPointer<KateDocument> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu*> subMenus;

  public  slots:
    void slotAboutToShow();

  private slots:
    void setType (int mode);
};

#endif
