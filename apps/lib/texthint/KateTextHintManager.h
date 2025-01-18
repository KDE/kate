/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "kateprivate_export.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>
#include <QObject>
#include <QPointer>

#include <vector>

namespace KTextEditor
{
class MainWindow;
class View;
class Cursor;
}
class KTETextHintProvider;
class KateTextHintView;

enum class TextHintMarkupKind {
    None = 0,
    PlainText = 1,
    MarkDown = 2,
};

class KATE_PRIVATE_EXPORT KateTextHintProvider : public QObject
{
    Q_OBJECT
public:
    KateTextHintProvider(KTextEditor::MainWindow *mainWindow, QObject *parent = nullptr);

Q_SIGNALS:
    // Emitted by KateTextHintManager, provider should prepare/request hint in response
    void textHintRequested(KTextEditor::View *v, KTextEditor::Cursor c);

    // Should be emitted by provider once it has text hint available
    void textHintAvailable(const QString &textHint, TextHintMarkupKind kind, KTextEditor::Cursor pos);

    // Should be emitted by provider if it wants to show text hint itself without a request
    void showTextHint(const QString &textHint, TextHintMarkupKind kind, KTextEditor::Cursor pos);
};

class KateTextHintManager : public QObject
{
    Q_OBJECT
public:
    explicit KateTextHintManager(KTextEditor::MainWindow *mainWindow);
    ~KateTextHintManager() override;

    enum class Requestor {
        HintProvider,
        CursorChange,
    };

    void ontextHintRequested(KTextEditor::View *v, KTextEditor::Cursor c, Requestor hintSource);

    void registerProvider(KateTextHintProvider *provider);

private:
    void showTextHint(size_t instanceId, const QString &hint, TextHintMarkupKind kind, KTextEditor::Cursor pos, bool force);
    KTextEditor::Range getLastRange(Requestor requestor);
    void setLastRange(KTextEditor::Range range, Requestor requestor);

private:
    // KTextEditor provider
    KTETextHintProvider *const m_provider;

    std::vector<KateTextHintProvider *> m_providers;

    // nullptr indicates absence of a hint view
    KateTextHintView *m_hintView;
    Requestor m_lastRequestor;

    KTextEditor::Range m_HintProviderLastRange = KTextEditor::Range::invalid();
    KTextEditor::Range m_CursorChangeLastRange = KTextEditor::Range::invalid();
};
