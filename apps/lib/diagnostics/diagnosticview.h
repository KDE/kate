/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include "kateprivate_export.h"

#include "diagnostic_types.h"

#include <QJsonObject>
#include <QPointer>
#include <QStandardItemModel>
#include <QUrl>
#include <QWidget>

#include <KXMLGUIClient>

#include <KTextEditor/Document>
#include <KTextEditor/Range>
#include <span>
#include <unordered_set>

class KConfigGroup;
class SessionDiagnosticSuppressions;
class KateMainWindow;
class QSortFilterProxyModel;
class KateTextHintProvider;

namespace KTextEditor
{
class MainWindow;
class Mark;
class View;
class MovingRange;
}

struct QUrlHash {
    std::size_t operator()(const QUrl &url) const
    {
        return qHash(url.toString());
    }
};

class KATE_PRIVATE_EXPORT DiagnosticsProvider : public QObject
{
    Q_OBJECT
public:
    explicit DiagnosticsProvider(KTextEditor::MainWindow *mainWindow, QObject *parent = nullptr);

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

    /**
     * If @p filterTo is a valid provider than DiagnosticView will
     * filter out all diagnostics that are not from @p filterTo.
     */
    void showDiagnosticsView(DiagnosticsProvider *filterTo = nullptr);

    /**
     * If @p filterTo is a valid provider, then DiagnosticView will
     * filter out all diagnostics that are not from @p filterTo.
     */
    void filterDiagnosticsViewTo(DiagnosticsProvider *filterTo);

    /**
     * Whether diagnostics of this provider should be automatically cleared
     * when a document is closed
     */
    void setPersistentDiagnostics(bool p)
    {
        m_persistentDiagnostics = p;
    }

    bool persistentDiagnostics() const
    {
        return m_persistentDiagnostics;
    }

    QString name;

Q_SIGNALS:
    /// emitted by provider when diags are available
    void diagnosticsAdded(const FileDiagnostics &);

    /// emitted by provider
    /// DiagnosticView will remove diagnostics where provider is @p provider
    void requestClearDiagnostics(DiagnosticsProvider *provider);

    /// Request fixes for given diagnostic
    /// @p data must be passed back when fixes are sent back via fixesAvailable()
    /// emitted by DiagnosticView
    void requestFixes(const QUrl &, const Diagnostic &, const QVariant &data);

    /// emitted by provider when fixes are available
    void fixesAvailable(const QList<DiagnosticFix> &fixes, const QVariant &data);

    /// emitted by provider to clear suppressions
    /// (as some state that the previously provided ones depend on may have changed
    void requestClearSuppressions(DiagnosticsProvider *provider);

private:
    friend class DiagnosticsView;
    class DiagnosticsView *diagnosticView;
    bool m_persistentDiagnostics = false;
};

class DiagTabOverlay : public QWidget
{
public:
    DiagTabOverlay(QWidget *parent)
        : QWidget(parent)
        , m_tabButton(parent)
    {
        if (!parent) {
            hide();
            return;
        }
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setGeometry(parent->geometry());
        move({0, 0});
        show();
        raise();
    }

    void setActive(bool a)
    {
        if (m_tabButton && (m_active != a)) {
            m_active = a;
            if (m_tabButton->size() != size()) {
                resize(m_tabButton->size());
            }
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool m_active = false;
    QWidget *m_tabButton = nullptr;
};

class DiagnosticsView : public QWidget, public KXMLGUIClient
{
    Q_OBJECT
    friend class ForwardingTextHintProvider;

protected:
    explicit DiagnosticsView(QWidget *parent, KTextEditor::MainWindow *mainWindow);

public:
    static DiagnosticsView *instance(KTextEditor::MainWindow *mainWindow);
    ~DiagnosticsView();

    void registerDiagnosticsProvider(DiagnosticsProvider *provider);
    void unregisterDiagnosticsProvider(DiagnosticsProvider *provider);

    void readSessionConfig(const KConfigGroup &config);
    void writeSessionConfig(KConfigGroup &config);

    void onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position, bool manual) const;

    void showToolview(DiagnosticsProvider *filterTo = nullptr);
    void filterViewTo(DiagnosticsProvider *provider);

    void shutdownStarts()
    {
        m_inShutdown = true;
    }

protected:
    void showEvent(QShowEvent *e) override;
    void handleEsc(QEvent *e);

private Q_SLOTS:
    void tabForToolViewAdded(QWidget *toolView, QWidget *tab);

private:
    void onFixesAvailable(const QList<DiagnosticFix> &fixes, const QVariant &data);
    void showFixesInMenu(const QList<DiagnosticFix> &fixes);
    void quickFix();
    void moveDiagnosticsSelection(bool forward);
    void nextItem();
    void previousItem();
    void onDiagnosticsAdded(const FileDiagnostics &diagnostics);
    void clearDiagnosticsFromProvider(DiagnosticsProvider *provider);
    void clearDiagnosticsForStaleDocs(const std::pmr::unordered_set<QUrl, QUrlHash> &filesToKeep, DiagnosticsProvider *provider);
    void clearSuppressionsFromProvider(DiagnosticsProvider *provider);
    void onDocumentUrlChanged();
    void updateDiagnosticsState(struct DocumentDiagnosticItem *&topItem);
    void updateMarks(const std::span<QUrl> &urls = {});
    void goToItemLocation(QModelIndex index);

    void onViewChanged(KTextEditor::View *v);

    void onDoubleClicked(const QModelIndex &index, bool quickFix = false);

    void addMarks(KTextEditor::Document *doc, QStandardItem *item);
    void addMarksRec(KTextEditor::Document *doc, QStandardItem *item);
    void addMarks(KTextEditor::Document *doc);

    Q_SLOT void clearAllMarks(KTextEditor::Document *doc);
    Q_SLOT void onMarkClicked(KTextEditor::Document *document, KTextEditor::Mark mark, bool &handled);

    bool syncDiagnostics(KTextEditor::Document *document, KTextEditor::Cursor pos, bool allowTop, bool doShow);
    void updateDiagnosticsSuppression(struct DocumentDiagnosticItem *topItem, KTextEditor::Document *doc, bool force = false);

    void onContextMenuRequested(const QPoint &pos);

    void setupDiagnosticViewToolbar(class QVBoxLayout *mainLayout);

    int m_diagnosticsCount = 0;
    const QPointer<KTextEditor::MainWindow> m_mainWindow;
    class QTreeView *const m_diagnosticsTree;
    class QToolButton *const m_clearButton;
    class QLineEdit *const m_filterLineEdit;
    class QComboBox *const m_providerCombo;
    class QToolButton *const m_errFilterBtn;
    class QToolButton *const m_warnFilterBtn;
    class KMessageWidget *const m_diagLimitReachedWarning;

    class ProviderListModel *m_providerModel;

    QStandardItemModel m_model;
    QSortFilterProxyModel *const m_proxy;
    std::vector<DiagnosticsProvider *> m_providers;
    std::unique_ptr<SessionDiagnosticSuppressions> m_sessionDiagnosticSuppressions;

    QHash<KTextEditor::Document *, QList<KTextEditor::MovingRange *>> m_diagnosticsRanges;
    // applied marks
    QSet<KTextEditor::Document *> m_diagnosticsMarks;

    QPointer<DiagTabOverlay> m_tabButtonOverlay;

    QMetaObject::Connection posChangedConnection;
    QTimer *const m_posChangedTimer;
    QTimer *const m_filterChangedTimer;
    QTimer *const m_urlChangedTimer;
    std::unique_ptr<KateTextHintProvider> m_textHintProvider;
    int m_diagnosticLimit = 0;

    // are we in main window shutdown and will soon be deleted?
    bool m_inShutdown = false;
};
