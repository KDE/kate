#pragma once

#include "KateTextHintManager.h"
#include "hintstate.h"

#include <QTextEdit>

class KateTextHintView : public QObject
{
    Q_OBJECT
public:
    explicit KateTextHintView(KTextEditor::MainWindow *mainWindow, KateTextHintManager *parent);
    void setView(KTextEditor::View *view);
    void update(size_t instanceId, const QString &text, TextHintMarkupKind kind, KTextEditor::View *view);

private:
    void render();

    QWidget *m_pane;
    QTextEdit *m_edit;
    HintState m_state;
    KTextEditor::View *m_view;

    // To destroy the connections in a deterministic order when ~KateTextHintView() is called
    QMetaObject::Connection m_viewTracker;
    QMetaObject::Connection m_cursorTracker;
};
