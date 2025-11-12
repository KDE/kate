/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QApplication>
#include <QDesktopServices>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <optional>

class CommitInfoHighlighter : public QSyntaxHighlighter
{
public:
    using QSyntaxHighlighter::QSyntaxHighlighter;

    void highlightBlock(const QString &text) override
    {
        highlightLinks(text);
    }

    void highlightLinks(QStringView text)
    {
        const auto schemes = {u"https://", u"http://"};
        for (const auto &scheme : schemes) {
            int start = text.indexOf(QStringView(scheme));
            if (start >= 0) {
                int nl = text.indexOf(u' ', start);
                nl = nl == -1 ? text.size() : nl;
                QTextCharFormat fmt;
                if (m_hoveredBlock && m_hoveredBlock.value() == currentBlock()) {
                    fmt.setForeground(qApp->palette().linkVisited());
                } else {
                    fmt.setForeground(qApp->palette().link());
                }
                fmt.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                setFormat(start, nl - start, fmt);
            }
        }
    }

    std::optional<QTextBlock> m_hoveredBlock;
};

class CommitInfoView : public QPlainTextEdit
{
public:
    explicit CommitInfoView(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
        , m_hl(document())
    {
        setMouseTracking(true);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier)) {
            QTextCursor c = cursorForPosition(e->pos());
            const QUrl url = QUrl(urlForText(c.block().text()));
            if (url.isValid()) {
                QDesktopServices::openUrl(url);
                return;
            }
        }

        QPlainTextEdit::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            QTextCursor c = cursorForPosition(e->pos());
            if (!c.isNull()) {
                auto block = c.block();
                const QUrl url = QUrl(urlForText(block.text()));
                if (url.isValid()) {
                    if (m_cursorChanged) {
                        return;
                    }
                    m_cursorChanged = true;
                    viewport()->setCursor(Qt::PointingHandCursor);
                    m_hl.m_hoveredBlock = block;
                    m_hl.rehighlightBlock(block);
                    return;
                }
            }
        }

        if (m_cursorChanged) {
            m_cursorChanged = false;
            if (m_hl.m_hoveredBlock.has_value()) {
                const auto block = m_hl.m_hoveredBlock.value();
                m_hl.m_hoveredBlock = std::nullopt;
                m_hl.rehighlightBlock(block);
            }
            viewport()->unsetCursor();
        }

        QPlainTextEdit::mousePressEvent(e);
    }

    static QString urlForText(const QString &text)
    {
        const auto schemes = {u"https://", u"http://"};
        for (const auto &scheme : schemes) {
            int start = text.indexOf(QStringView(scheme));
            if (start >= 0) {
                int nl = text.indexOf(u' ', start);
                nl = nl == -1 ? text.size() : nl;
                return text.mid(start, nl - start);
            }
        }
        return {};
    }

private:
    CommitInfoHighlighter m_hl;
    bool m_cursorChanged = false;
};
