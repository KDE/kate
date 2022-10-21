/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_BYTE_ARRAY_SPLITTER
#define KATE_BYTE_ARRAY_SPLITTER

#include <QByteArray>
#include <QString>

#include <charconv>
#include <optional>
#include <string_view>

class strview : public std::string_view
{
public:
    using std::string_view::string_view;
    strview(std::string_view s)
        : std::string_view(s)
    {
    }

    QString toString() const
    {
        return QString::fromUtf8(data(), size());
    }
    QByteArray toByteArray() const
    {
        return QByteArray(data(), size());
    }

    template<typename Int>
    std::optional<Int> to(Int &&value = {}) const
    {
        auto res = std::from_chars(data(), data() + size(), value);
        if (res.ptr == (data() + size())) {
            return value;
        }
        return std::nullopt;
    }
};

template<typename ArrayType>
class ByteArraySplitter
{
public:
    ByteArraySplitter(const ArrayType &b, char splitChar)
        : data(b)
        , s(splitChar)
    {
    }

    class iterator
    {
    public:
        iterator()
        {
        }

        iterator(const char *c, size_t size, char split)
            : sv(c, size)
            , s(size == 0 ? std::string::npos : 0)
            , sp(split)
        {
            e = sv.find(sp, s);
        }

        iterator &operator++()
        {
            auto newPos = sv.find(sp, e == std::string::npos ? e : e + 1);
            s = e != std::string::npos ? e + 1 : std::string::npos;
            e = newPos;
            return *this;
        }

        iterator operator++(int)
        {
            auto old = *this;
            auto newPos = sv.find(sp, e == std::string::npos ? e : e + 1);
            s = e != std::string::npos ? e + 1 : std::string::npos;
            e = newPos;
            return old;
        }

        strview operator*() const
        {
            if (s != std::string::npos) {
                return sv.substr(s, e != std::string::npos ? (e - s) : e);
            }
            return std::string_view(nullptr, 0);
        }

        bool operator==(const iterator &r) const
        {
            return s == r.s && e == r.e && sp == r.sp;
        }

        bool operator!=(const iterator &r) const
        {
            return !(*this == r);
        }

    private:
        strview sv;
        std::size_t s;
        std::size_t e;
        char sp;
    };

    iterator begin() const
    {
        return iterator(data.data(), data.size(), s);
    }

    iterator end() const
    {
        return iterator(nullptr, 0, s);
    }

    bool empty() const
    {
        return begin() == end();
    }

    template<typename ContainerType>
    ContainerType toContainer(ContainerType &&c = {})
    {
        for (auto e : *this)
            c.push_back(e);
        return std::forward<ContainerType>(c);
    }

private:
    const ArrayType &data;
    char s;
};

#endif
