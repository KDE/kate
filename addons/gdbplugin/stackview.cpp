/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-FileCopyrightText: 2025 Kåre Särs <kare.sars@iki.fi>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "stackview.h"
#include "stackframe_model.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KTextEditor/Range>

#include <ktexteditor_utils.h>

StackView::StackView(Backend *backend, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_backend(backend)
    , m_mainWindow(mainWindow)
    , m_threadCombo(new QComboBox(this))
    , m_stackTree(new QTreeView(this))
{
    connect(m_backend, &BackendInterface::stackFrameInfo, this, &StackView::insertStackFrame);
    connect(m_backend, &BackendInterface::stackFrameChanged, this, &StackView::stackFrameChanged);
    connect(m_backend, &BackendInterface::threads, this, &StackView::onThreads);
    connect(m_backend, &BackendInterface::threadUpdated, this, &StackView::updateThread);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(m_threadCombo);
    layout->addWidget(m_stackTree);

    m_stackTree->setModel(new StackFrameModel(this));
    m_stackTree->setRootIsDecorated(false);
    m_stackTree->resizeColumnToContents(0);
    m_stackTree->resizeColumnToContents(1);
    m_stackTree->setAutoScroll(false);
    m_stackTree->setUniformRowHeights(true);
    connect(m_stackTree, &QTreeView::activated, this, &StackView::stackFrameSelected);
    m_stackTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_stackTree, &QTreeView::customContextMenuRequested, this, &StackView::onStackTreeContextMenuRequest);

    connect(m_threadCombo, &QComboBox::currentIndexChanged, this, &StackView::threadSelected);
}

void StackView::clear()
{
    auto *model = static_cast<StackFrameModel *>(m_stackTree->model());
    model->setFrames({}, {});
    model->setActiveFrame(-1);
    m_threadCombo->clear();
}

void StackView::insertStackFrame(const QList<dap::StackFrame> &frames)
{
    auto model = static_cast<StackFrameModel *>(m_stackTree->model());
    model->setFrames(frames, m_backend->modules());
    m_stackTree->resizeColumnToContents(StackFrameModel::Column::Path);
}

void StackView::stackFrameSelected()
{
    m_backend->changeStackFrame(m_stackTree->currentIndex().row());
}

void StackView::stackFrameChanged(int level)
{
    auto model = static_cast<StackFrameModel *>(m_stackTree->model());
    model->setActiveFrame(level);
}

void StackView::onThreads(const QList<dap::Thread> &threads)
{
    // We dont want to signal thread change while we are modifying the combo
    disconnect(m_threadCombo, &QComboBox::currentIndexChanged, this, &StackView::threadSelected);
    m_threadCombo->clear();
    int activeThread = m_activeThread;
    m_activeThread = -1;
    bool oldActiveThreadFound = false;

    const auto pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
    for (const auto &thread : threads) {
        QString name = i18n("Thread %1", thread.id);
        if (!thread.name.isEmpty()) {
            name += QStringLiteral(": %1").arg(thread.name);
        }
        QPixmap icon = pix;
        if (thread.id == activeThread) {
            icon = QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10);
            oldActiveThreadFound = true;
        }
        m_threadCombo->addItem(icon, name, thread.id);
    }

    connect(m_threadCombo, &QComboBox::currentIndexChanged, this, &StackView::threadSelected);
    // try to set an active thread
    if (m_threadCombo->count() > 0) {
        int idx = 0; // use first thread
        if (oldActiveThreadFound) {
            // if old active thread was found, use that instead
            idx = m_threadCombo->findData(activeThread);
            m_activeThread = activeThread;
        } else {
            m_activeThread = m_threadCombo->itemData(idx).toInt();
        }
        m_threadCombo->setCurrentIndex(idx);
    }
}

void StackView::updateThread(const dap::Thread &thread, Backend::ThreadState state, bool isActive)
{
    int idx = m_threadCombo->findData(thread.id);
    if (idx == -1 && state != Backend::ThreadState::Exited) {
        // thread wasn't found, add it
        QString name = i18n("Thread %1", thread.id);
        const QPixmap pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
        m_threadCombo->addItem(pix, name, thread.id);
    } else if (idx != -1 && state == Backend::ThreadState::Exited) {
        // remove exited thread
        m_threadCombo->removeItem(idx);
    }

    // Try to set an active thread
    if (isActive) {
        if (m_activeThread != thread.id && m_activeThread != -1) {
            int oldActiveIdx = m_threadCombo->findData(m_activeThread);
            const QPixmap pix = QIcon::fromTheme(QLatin1String("")).pixmap(10, 10);
            m_threadCombo->setItemIcon(oldActiveIdx, QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10));
        }

        m_activeThread = thread.id;
        m_threadCombo->setItemIcon(idx, QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(10, 10));
        m_threadCombo->setCurrentIndex(idx);
    }

    if (m_activeThread == -1 && m_threadCombo->count() > 0) {
        // activate first thread if nothing active
        m_activeThread = m_threadCombo->itemData(0).toInt();
        m_threadCombo->setCurrentIndex(0);
    }
}

void StackView::threadSelected(int thread)
{
    if (thread < 0)
        return;
    m_backend->changeThread(m_threadCombo->itemData(thread).toInt());
}

void StackView::onStackTreeContextMenuRequest(QPoint pos)
{
    QMenu menu(m_stackTree);
    auto model = m_stackTree->model();

    auto a = menu.addAction(i18n("Copy Stack Trace"));
    connect(a, &QAction::triggered, m_stackTree, [model] {
        QString text;
        for (int i = 0; i < model->rowCount(); ++i) {
            QString no = model->index(i, StackFrameModel::Column::Number).data().toString();
            QString func = model->index(i, StackFrameModel::Column::Func).data().toString();
            QString path = model->index(i, StackFrameModel::Column::Path).data().toString();
            text.append(QStringLiteral("#%1  %2  %3\n").arg(no, func, path));
        }
        qApp->clipboard()->setText(text);
    });

    auto index = m_stackTree->currentIndex();
    if (index.isValid()) {
        auto frame = index.data(StackFrameModel::StackFrameRole).value<dap::StackFrame>();
        if (frame.source) {
            auto url = frame.source->path;
            int line = frame.line - 1;
            if (url.isValid()) {
                auto a = menu.addAction(i18n("Open Location"));
                connect(a, &QAction::triggered, m_stackTree, [this, url, line] {
                    Utils::goToDocumentLocation(m_mainWindow, url, KTextEditor::Range({line, 0}, 0), {.focus = true, .record = false});
                });
            }
        }

        auto a = menu.addAction(i18n("Copy Location"));
        QString location = QStringLiteral("%1:%2").arg(Utils::formatUrl(frame.source->path)).arg(frame.line);
        connect(a, &QAction::triggered, m_stackTree, [location] {
            qApp->clipboard()->setText(location);
        });
    }

    menu.exec(m_stackTree->viewport()->mapToGlobal(pos));
}
