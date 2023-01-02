/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#ifndef KATE_DIAGNOSTICS_VIEW
#define KATE_DIAGNOSTICS_VIEW

#include "kateprivate_export.h"

#include "diagnostic_types.h"

#include <QJsonObject>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUrl>
#include <QWidget>

#include <KTextEditor/MarkInterface>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Range>

class KConfigGroup;
class SessionDiagnosticSuppressions;
class KateMainWindow;

typedef QMultiHash<KTextEditor::Document *, KTextEditor::MovingRange *> RangeCollection;
typedef QSet<KTextEditor::Document *> DocumentCollection;

class KATE_PRIVATE_EXPORT DiagnosticsProvider : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    // Get suppressions
    // e.g json object
    /*
     * "suppressions": {
     *     "rulename": ["filename_regex", "message regexp", "code regexp"],
     *  }
     */
    virtual QJsonObject suppressions(KTextEditor::Document *) const
    {
        return {};
    }

    bool hasTooltipForPos(KTextEditor::View *v, KTextEditor::Cursor pos) const;

Q_SIGNALS:
    /// emitted by provider when diags are available
    void diagnosticsAdded(const FileDiagnostics &);

    /// emitted by provider
    /// DiagnosticView will remove diagnostics where provider is @p provider
    /// and filepath of a diagnostic is not contained in @p filesToKeep
    void requestClearDiagnosticsForStaleDocs(const QVector<QString> &filesToKeep, DiagnosticsProvider *provider);

    /// Request fixes for given diagnostic
    /// @p data must be passed back when fixes are sent back via fixesAvailable()
    /// emitted by DiagnosticView
    void requestFixes(const QUrl &, const Diagnostic &, const QVariant &data);

    /// emitted by provider when fixes are available
    void fixesAvailable(const QVector<DiagnosticFix> &fixes, const QVariant &data);

private:
    friend class DiagnosticsView;
    class DiagnosticsView *diagnosticView;
};

class DiagnosticsView : public QWidget
{
    Q_OBJECT
    friend class ForwardingTextHintProvider;

public:
    explicit DiagnosticsView(QWidget *parent, KateMainWindow *mainWindow);
    ~DiagnosticsView();

    void registerDiagnosticsProvider(DiagnosticsProvider *provider);
    void unregisterDiagnosticsProvider(DiagnosticsProvider *provider);

    void readSessionConfig(const KConfigGroup &config);
    void writeSessionConfig(KConfigGroup &config);

    QString onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position) const;

private:
    void onFixesAvailable(const QVector<DiagnosticFix> &fixes, const QVariant &data);
    void showFixesInMenu(const QVector<DiagnosticFix> &fixes);
    void quickFix();
    void onDiagnosticsAdded(const FileDiagnostics &diagnostics);
    void clearDiagnosticsForStaleDocs(const QVector<QString> &filesToKeep, DiagnosticsProvider *provider);
    void updateDiagnosticsState(QStandardItem *topItem);
    void updateMarks();
    void goToItemLocation(QModelIndex index);

    void onViewChanged(KTextEditor::View *v);

    void onDoubleClicked(const QModelIndex &index, bool quickFix = false);

    void addMarks(KTextEditor::Document *doc, QStandardItem *item);
    void addMarksRec(KTextEditor::Document *doc, QStandardItem *item);
    void addMarks(KTextEditor::Document *doc);

    Q_SLOT void clearAllMarks(KTextEditor::Document *doc);
    Q_SLOT void onMarkClicked(KTextEditor::Document *document, KTextEditor::Mark mark, bool &handled);

    bool syncDiagnostics(KTextEditor::Document *document, int line, bool allowTop, bool doShow);
    void updateDiagnosticsSuppression(QStandardItem *topItem, KTextEditor::Document *doc, bool force = false);

    void onContextMenuRequested(const QPoint &pos);

    KateMainWindow *const m_mainWindow;
    QTreeView *const m_diagnosticsTree;
    QStandardItemModel m_model;
    QVector<DiagnosticsProvider *> m_providers;
    std::unique_ptr<SessionDiagnosticSuppressions> m_sessionDiagnosticSuppressions;

    RangeCollection m_diagnosticsRanges;
    // applied marks
    DocumentCollection m_diagnosticsMarks;

    class ForwardingTextHintProvider *m_textHintProvider;

    QMetaObject::Connection posChangedConnection;
    QTimer *const m_posChangedTimer;
};

#endif
