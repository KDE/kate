/*
    SPDX-FileCopyrightText: 2004 Anders Lund <anders@alweb.dk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>
#include <QVector>

class KProcess;
class QTemporaryFile;
class QTreeWidget;
class QTreeWidgetItem;

namespace KTextEditor
{
class Document;
}

/**
 * A dialog for handling multiple documents modified on disk
 * from within KateMainWindow
 */
class KateMwModOnHdDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KateMwModOnHdDialog(const QVector<KTextEditor::Document *> &docs, QWidget *parent = nullptr, const char *name = nullptr);
    ~KateMwModOnHdDialog() override;
    void addDocument(KTextEditor::Document *doc);

    void setShowOnWindowActivation(bool show)
    {
        m_showOnWindowActivation = show;
    }
    bool showOnWindowActivation() const
    {
        return m_showOnWindowActivation;
    }

Q_SIGNALS:
    void requestOpenDiffDocument(const QUrl &documentUrl);

private Q_SLOTS:
    void slotIgnore();
    void slotOverwrite();
    void slotReload();
    void slotDiff();
    void slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
    void slotCheckedFilesChanged(QTreeWidgetItem *, int column);
    void slotGitDiffDone(class QProcess *p, KTextEditor::Document *doc);

public Q_SLOTS:
    void removeDocument(QObject *doc);

private:
    enum Action { Ignore, Overwrite, Reload };
    void handleSelected(int action);
    class QTreeWidget *docsTreeWidget;
    class QDialogButtonBox *dlgButtons;
    class QPushButton *btnDiff;
    QStringList m_stateTexts;
    bool m_blockAddDocument;
    bool m_showOnWindowActivation = false;

protected:
    void closeEvent(QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *) override;
};
