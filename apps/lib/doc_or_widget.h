#ifndef KATE_DOC_OR_WIDGET
#define KATE_DOC_OR_WIDGET

#include <KTextEditor/Document>
#include <QWidget>

#include <variant>

// TODO: Find a better name for this class

// Just a helper class which we use internally to manage widgets/docs
class DocOrWidget : public std::variant<KTextEditor::Document *, QWidget *>
{
public:
    using variant::variant;

    auto *doc() const
    {
        return std::holds_alternative<KTextEditor::Document *>(*this) ? std::get<KTextEditor::Document *>(*this) : nullptr;
    }

    auto *widget() const
    {
        return std::holds_alternative<QWidget *>(*this) ? std::get<QWidget *>(*this) : nullptr;
    }

    QObject *qobject() const
    {
        return doc() ? static_cast<QObject *>(doc()) : static_cast<QObject *>(widget());
    }

    bool operator==(KTextEditor::Document *doc) const
    {
        return this->doc() == doc;
    }
    bool operator==(QWidget *w) const
    {
        return this->widget() == w;
    }

    static DocOrWidget null()
    {
        DocOrWidget d = static_cast<KTextEditor::Document *>(nullptr);
        return d;
    }

    bool isNull() const
    {
        return !qobject();
    }

    void clear()
    {
        *this = null();
    }
};
Q_DECLARE_METATYPE(DocOrWidget)

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

#endif
