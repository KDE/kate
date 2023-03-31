/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "KateTextHintManager.h"

#include "katemainwindow.h"
#include "tooltip.h"

#include <KTextEditor/MainWindow>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/View>

KateTextHintProvider::KateTextHintProvider(KTextEditor::MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(mainWindow);
    auto mgr = static_cast<KateMainWindow *>(mainWindow->window())->textHintManager();
    mgr->registerProvider(this);
}

class KTETextHintProvider : public KTextEditor::TextHintProvider
{
    KateTextHintManager *m_mgr;
    QPointer<KTextEditor::View> m_view;

public:
    KTETextHintProvider(KateTextHintManager *mgr)
        : m_mgr(mgr)
    {
        Q_ASSERT(m_mgr);
    }

    ~KTETextHintProvider()
    {
        if (m_view) {
            auto iface = qobject_cast<KTextEditor::TextHintInterface *>(m_view);
            iface->unregisterTextHintProvider(this);
        }
    }

    void setView(KTextEditor::View *v)
    {
        if (m_view) {
            auto iface = qobject_cast<KTextEditor::TextHintInterface *>(m_view);
            iface->unregisterTextHintProvider(this);
        }
        if (v) {
            m_view = v;
            auto iface = qobject_cast<KTextEditor::TextHintInterface *>(m_view);
            iface->registerTextHintProvider(this);
        }
    }

    KTextEditor::View *view() const
    {
        return m_view;
    }

    virtual QString textHint(KTextEditor::View *view, const KTextEditor::Cursor &position) override
    {
        m_mgr->ontextHintRequested(view, position);
        return {};
    }
};

KateTextHintManager::KateTextHintManager(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_provider(new KTETextHintProvider(this))
{
    connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *v) {
        m_provider->setView(v);
    });
}

KateTextHintManager::~KateTextHintManager()
{
    m_provider->setView(nullptr);
    delete m_provider;
}

void KateTextHintManager::registerProvider(KateTextHintProvider *provider)
{
    if (std::find(m_providers.begin(), m_providers.end(), provider) == m_providers.end()) {
        m_providers.push_back(provider);
        connect(provider, &QObject::destroyed, this, [this](QObject *o) {
            unregisterProvider(static_cast<KateTextHintProvider *>(o));
        });
        connect(provider, &KateTextHintProvider::textHintAvailable, this, &KateTextHintManager::onTextHintAvailable);
        connect(provider, &KateTextHintProvider::showTextHint, this, &KateTextHintManager::onShowTextHint);
    }
}
void KateTextHintManager::unregisterProvider(KateTextHintProvider *provider)
{
    auto it = std::find(m_providers.begin(), m_providers.end(), provider);
    if (it != m_providers.end()) {
        m_providers.erase(it);
    }
}

void KateTextHintManager::ontextHintRequested(KTextEditor::View *v, KTextEditor::Cursor c)
{
    for (auto provider : m_providers) {
        Q_EMIT provider->textHintRequested(v, c);
    }
}

void KateTextHintManager::showTextHint(const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos, bool force)
{
    if (QStringView(hint).trimmed().isEmpty() || !pos.isValid()) {
        return;
    }
    auto view = m_provider->view();
    if (!view) {
        return;
    }

    QPoint p = view->cursorToCoordinate(pos);
    KateTooltip::show(hint, kind, view->mapToGlobal(p), view, force);
}

void KateTextHintManager::onTextHintAvailable(const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos)
{
    showTextHint(hint, kind, pos, false);
}

void KateTextHintManager::onShowTextHint(const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos)
{
    showTextHint(hint, kind, pos, true);
}
