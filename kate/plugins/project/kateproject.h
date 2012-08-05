/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_PROJECT_H
#define KATE_PROJECT_H

#include <QObject>
#include <QStandardItemModel>

/**
 * Class representing a project.
 * Holds project properties like name, groups, contained files, ...
 */
class KateProject : public QObject
{
  Q_OBJECT

  public:
    /**
     * construct empty project
     */
    KateProject (QObject *parent);
    
    /**
     * deconstruct project
     */
    ~KateProject ();
    
    /**
     * Load a project.
     * Only works once, afterwards use reload().
     * @param fileName name of project file
     * @return success
     */
    bool load (const QString &fileName);
    
    /**
     * Try to reload a project.
     * If the reload fails, e.g. because the file is not readable or corrupt, nothing will happen!
     * @return success
     */
    bool reload ();
    
  private:
    /**
     * project file name
     */
    QString m_fileName;
    
    /**
     * project name
     */
    QString m_name;
    
    /**
     * standard item model with content of this project
     */
    QStandardItemModel *m_model;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
