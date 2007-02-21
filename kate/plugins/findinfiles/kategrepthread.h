/* This file is part of the KDE project
   Copyright (C) 2007 Christoph Cullmann <cullmann@kde.org>
 
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

#ifndef _GREPTHREAD_H_
#define _GREPTHREAD_H_

#include <QThread>
#include <QWidget>
#include <QRegExp>
#include <QList>
#include <QStringList>

class KateGrepThread : public QThread
{
    Q_OBJECT

  public:
    KateGrepThread (QWidget *parent, const QString &dir,
                    bool recursive, const QStringList &fileWildcards,
                    const QList<QRegExp> &searchPattern);
    ~KateGrepThread ();

  public:
    void run();
    void cancel ()
    {
      m_cancel = true;
    }

  private:
    void grepInFile (const QString &fileName, const QString &baseName);

  Q_SIGNALS:
    void foundMatch (const QString &filename, int line, int column, const QString &basename, const QString &lineContent);
    void finished ();

  private:
    bool m_cancel;
    QStringList m_workQueue;
    bool m_recursive;
    QStringList m_fileWildcards;
    QList<QRegExp> m_searchPattern;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
