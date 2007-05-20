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

#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtCore/QHash>

#include "katedialogs.h"

class KateDocument;

namespace Ui
{
  class FileTypeConfigWidget;
}

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
    QString hl;
    bool hlGenerated;
    QString version;
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

    void save (const QList<KateFileType *>& v);

    /**
     * get the right fileType for the given document
     * -1 if none found !
     */
    QString fileType (KateDocument *doc);

    /**
     * Don't store the pointer somewhere longer times, won't be valid after the next update()
     */
    const KateFileType& fileType (const QString &name) const;

    /**
     * Don't modify
     */
    const QList<KateFileType *>& list() const { return m_types; }

  private:
    QString wildcardsFind (const QString &fileName);

  private:
    QList<KateFileType *> m_types;
    QHash<QString, KateFileType *> m_name2Type;
};

class KateFileTypeConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    explicit KateFileTypeConfigTab( QWidget *parent );
    ~KateFileTypeConfigTab ();

  public Q_SLOTS:
    void apply();
    void reload();
    void reset();
    void defaults();

  private Q_SLOTS:
    void update ();
    void deleteType ();
    void newType ();
    void typeChanged (int type);
    void showMTDlg();
    void save ();

  private:
    Ui::FileTypeConfigWidget *ui;

    QList<KateFileType *> m_types;
    int m_lastType;
};

class KateViewFileTypeAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewFileTypeAction(const QString& text, QObject *parent)
       : KActionMenu(text, parent) { init(); }

    ~KateViewFileTypeAction();

    void updateMenu (KTextEditor::Document *doc);

  private:
    void init();

    QPointer<KateDocument> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu*> subMenus;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setType (QAction*);
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
