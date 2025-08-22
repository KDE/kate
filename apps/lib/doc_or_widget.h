/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Document>
#include <QWidget>

// Just a helper class which we use internally to manage widgets/docs
class DocOrWidget
{
    friend class DocOrWidgetTest;

private:
    // The "tag" to discriminate which union member is active.
    enum class Type {
        None,
        Document,
        Widget
    };

    Type m_type;
    union {
        KTextEditor::Document *m_doc;
        QWidget *m_widget;
    };

public:
    constexpr DocOrWidget()
        : m_type(Type::None)
        , m_doc(nullptr)
    {
    }
    constexpr DocOrWidget(KTextEditor::Document *doc)
        : m_type(Type::Document)
        , m_doc(doc)
    {
    }
    constexpr DocOrWidget(QWidget *widget)
        : m_type(Type::Widget)
        , m_widget(widget)
    {
    }

    constexpr DocOrWidget &operator=(KTextEditor::Document *doc)
    {
        m_type = Type::Document;
        m_doc = doc;
        return *this;
    }

    constexpr DocOrWidget &operator=(QWidget *widget)
    {
        m_type = Type::Widget;
        m_widget = widget;
        return *this;
    }

    constexpr KTextEditor::Document *doc() const
    {
        return (m_type == Type::Document) ? m_doc : nullptr;
    }

    constexpr QWidget *widget() const
    {
        return (m_type == Type::Widget) ? m_widget : nullptr;
    }

    constexpr QObject *qobject() const
    {
        switch (m_type) {
        case Type::Document:
            return m_doc;
        case Type::Widget:
            return m_widget;
        case Type::None:
            return nullptr;
        }
        Q_UNREACHABLE();
        return nullptr;
    }

    constexpr bool operator==(KTextEditor::Document *doc) const
    {
        return this->doc() == doc;
    }
    constexpr bool operator==(QWidget *w) const
    {
        return this->widget() == w;
    }
    constexpr bool operator==(DocOrWidget other) const
    {
        return m_type == other.m_type && qobject() == other.qobject();
    }

    constexpr bool isNull() const
    {
        return qobject() == nullptr;
    }

    void clear()
    {
        m_type = Type::None;
        m_doc = nullptr;
    }
};

namespace std
{
template<>
struct hash<DocOrWidget> {
    typedef DocOrWidget argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const &s) const noexcept
    {
        result_type const h1(std::hash<void *>{}(s.qobject()));
        return h1;
    }
};
}
