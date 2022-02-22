/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_URL_BAR_H
#define KATE_URL_BAR_H

#include "kateviewspace.h"

#include <QFrame>
#include <QPointer>

class KateUrlBar : public QWidget
{
    Q_OBJECT
public:
    explicit KateUrlBar(KateViewSpace *parent = nullptr);
    void open();

    class KateViewManager *viewManager();
    class KateViewSpace *viewSpace();
    QIcon seperator();

private:
    void setupLayout();

    void onViewChanged(KTextEditor::View *v);
    void updateForDocument(KTextEditor::Document *doc);
    class QStackedWidget *const m_stack;
    class UrlbarContainer *const m_urlBarView;
    class QLabel *const m_untitledDocLabel;
    class KateViewSpace *m_parentViewSpace;

    // document for which the url bar is currently active
    // might be nullptr
    QPointer<KTextEditor::Document> m_currentDoc;

Q_SIGNALS:
    void openUrlRequested(const QUrl &url, Qt::KeyboardModifiers);
};

#endif
