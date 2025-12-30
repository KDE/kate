/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "KateTextHintManager.h"

#include "hintview.h"
#include "tooltip.h"
#include <kateapp.h>

#include <KSharedConfig>
#include <KTextEditor/MainWindow>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/View>

KateTextHintProvider::KateTextHintProvider(KTextEditor::MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(mainWindow);
    auto mgr = static_cast<KateTextHintManager *>(mainWindow->property("textHintManager").value<QObject *>());
    if (!mgr) {
        mgr = new KateTextHintManager(mainWindow);
        mainWindow->setProperty("textHintManager", QVariant::fromValue(mgr));
    }
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

    ~KTETextHintProvider() override
    {
        if (m_view) {
            m_view->unregisterTextHintProvider(this);
        }
    }

    void setView(KTextEditor::View *v)
    {
        if (m_view) {
            m_view->unregisterTextHintProvider(this);
        }
        if (v) {
            m_view = v;
            m_view->registerTextHintProvider(this);
        }
    }

    KTextEditor::View *view() const
    {
        return m_view;
    }

    QString textHint(KTextEditor::View *view, const KTextEditor::Cursor &position) override
    {
        m_mgr->ontextHintRequested(view, position, KateTextHintManager::Requestor::HintProvider);
        return {};
    }
};

KateTextHintManager::KateTextHintManager(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_provider(new KTETextHintProvider(this))
    , m_hintView(nullptr)
{
    connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *v) {
        m_provider->setView(v);
    });

    const auto updateConfig = [this, mainWindow] {
        KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
        const auto hintViewEnabled = cgGeneral.readEntry("Enable Context ToolView", false);
        if (hintViewEnabled && !m_hintView) {
            m_hintView = new KateTextHintView(mainWindow, this);
        } else if (!hintViewEnabled && m_hintView) {
            delete m_hintView;
            m_hintView = nullptr;
        }
    };

    connect(KateApp::self(), &KateApp::configurationChanged, this, updateConfig);
    updateConfig();
}

KateTextHintManager::~KateTextHintManager()
{
    m_provider->setView(nullptr);
    delete m_hintView;
    delete m_provider;
}

void KateTextHintManager::registerProvider(KateTextHintProvider *provider)
{
    if (std::find(m_providers.begin(), m_providers.end(), provider) == m_providers.end()) {
        m_providers.push_back(provider);
        connect(provider, &QObject::destroyed, this, [this](QObject *destroyedProvider) {
            auto it = std::find(m_providers.begin(), m_providers.end(), destroyedProvider);
            if (it != m_providers.end()) {
                m_providers.erase(it);
            }
        });

        const auto slot = [provider, this](bool forced) {
            return [this, provider, forced](const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos) {
                const auto instanceId = reinterpret_cast<std::uintptr_t>(provider);
                if (forced) { // Forced requests go implicitly to the tooltip
                    showTextHint(instanceId, hint, kind, pos, true);
                    return;
                }
                const auto lastRange = getLastRange(m_lastRequestor);
                const auto posRange = m_provider->view()->document()->wordRangeAt(pos);
                // Ensure we're handling the range of last requestor
                if (lastRange == posRange) {
                    if (m_lastRequestor == Requestor::CursorChange && m_hintView) {
                        m_hintView->update(instanceId, hint, kind, m_provider->view());
                        return;
                    }
                    showTextHint(instanceId, hint, kind, pos, forced);
                }
            };
        };
        connect(provider, &KateTextHintProvider::textHintAvailable, this, slot(false));
        connect(provider, &KateTextHintProvider::showTextHint, this, slot(true));
    }
}

void KateTextHintManager::ontextHintRequested(KTextEditor::View *v, KTextEditor::Cursor c, Requestor hintSource)
{
    auto wordRange = v->document()->wordRangeAt(c);
    auto lastRange = getLastRange(hintSource);

    // avoid requesting if the range is same
    if (wordRange == lastRange) {
        return;
    }

    setLastRange(wordRange, hintSource);
    m_lastRequestor = hintSource;

    for (const auto &provider : m_providers) {
        Q_EMIT provider->textHintRequested(v, c);
    }
}

void KateTextHintManager::showTextHint(size_t instanceId, const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos, bool force)
{
    if (!pos.isValid()) {
        return;
    }

    auto view = m_provider->view();
    if (!view) {
        return;
    }

    QPoint p = view->cursorToCoordinate(pos);
    auto tooltip = KateTooltip::show(instanceId, hint, kind, view->mapToGlobal(p), view, force, getLastRange(Requestor::HintProvider));
    if (tooltip) {
        // unset the range if the tooltip is gone
        connect(tooltip, &QObject::destroyed, this, [this] {
            setLastRange(KTextEditor::Range::invalid(), Requestor::HintProvider);
        });
    }
}

void KateTextHintManager::setLastRange(KTextEditor::Range range, Requestor requestor)
{
    switch (requestor) {
    case Requestor::HintProvider:
        m_HintProviderLastRange = range;
        break;

    case Requestor::CursorChange:
        m_CursorChangeLastRange = range;
        break;
    }
}

KTextEditor::Range KateTextHintManager::getLastRange(Requestor requestor)
{
    switch (requestor) {
    case Requestor::HintProvider:
        return m_HintProviderLastRange;

    case Requestor::CursorChange:
        return m_CursorChangeLastRange;
    }

    Q_ASSERT(false);
    return KTextEditor::Range::invalid();
}

#include "moc_KateTextHintManager.cpp"
