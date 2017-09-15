/*
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_MW_MODONHD_DIALOG_H_
#define _KATE_MW_MODONHD_DIALOG_H_

#include <ktexteditor/document.h>

#include <QVector>
#include <QDialog>

class KProcess;
class QTemporaryFile;
class QTreeWidget;
class QTreeWidgetItem;

typedef  QVector<KTextEditor::Document *> DocVector;

/**
 * A dialog for handling multiple documents modified on disk
 * from within KateMainWindow
 */
class KateMwModOnHdDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KateMwModOnHdDialog(DocVector docs, QWidget *parent = nullptr, const char *name = nullptr);
    ~KateMwModOnHdDialog() override;
    void addDocument(KTextEditor::Document *doc);

private Q_SLOTS:
    void slotIgnore();
    void slotOverwrite();
    void slotReload();
    void slotDiff();
    void slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
    void slotDataAvailable();
    void slotPDone();

private:
    enum Action { Ignore, Overwrite, Reload };
    void handleSelected(int action);
    class QTreeWidget *twDocuments;
    class QPushButton *btnDiff;
    KProcess *m_proc;
    QTemporaryFile *m_diffFile;
    QStringList m_stateTexts;
    bool m_blockAddDocument;

protected:
    void closeEvent(QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *) override;

};

#endif // _KATE_MW_MODONHD_DIALOG_H_
