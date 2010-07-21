/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *  Copyright (C) 2010 Diana-Victoria Tiriplica <diana.tiriplica@gmail.com>
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

#ifndef KATE_SWAPFILE_H
#define KATE_SWAPFILE_H

#include <QtCore/QObject>
#include <QtCore/QDataStream>
#include <QFile>
#include <QTimer>

#include "katepartprivate_export.h"
#include "katetextbuffer.h"
#include "katebuffer.h"
#include "katedocument.h"

class KateView;

namespace Kate {

/**
 * Class for tracking editing actions.
 * In case Kate crashes, this can be used to replay all edit actions to
 * recover the lost data.
 */
class KATEPART_TESTS_EXPORT SwapFile : public QObject
{
  Q_OBJECT

  public:
    explicit SwapFile(KateDocument* document);
    ~SwapFile();
    bool shouldRecover() const;
    
    void fileClosed ();
    QString fileName();
    
  private:
    void setTrackingEnabled(bool trackingEnabled);
    void removeSwapFile();
    bool updateFileName();

  private:
    KateDocument *m_document;
    bool m_trackingEnabled;

  protected Q_SLOTS:
    void fileSaved(const QString& filename);
    void fileLoaded(const QString &filename);

    void startEditing ();
    void finishEditing ();

    void wrapLine (const KTextEditor::Cursor &position);
    void unwrapLine (int line);
    void insertText (const KTextEditor::Cursor &position, const QString &text);
    void removeText (const KTextEditor::Range &range);
    
  Q_SIGNALS:
    void swapFileFound();
    void swapFileHandled();
  
  public Q_SLOTS:
    void discard();
    void recover();
    bool recover(QDataStream&);

  private:
    QDataStream m_stream;
    QFile m_swapfile;
    bool m_recovered;
    bool m_modified;
    static QTimer *s_timer;

  protected Q_SLOTS:
    void writeFileToDisk();

  private:
    QTimer* syncTimer();
};

}

#endif // KATE_SWAPFILE_H

// kate: space-indent on; indent-width 2; replace-tabs on;
