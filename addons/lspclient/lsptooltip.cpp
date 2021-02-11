/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "lsptooltip.h"

#include <QDebug>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QString>
#include <QTextBrowser>
#include <QTimer>

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/State>

using KSyntaxHighlighting::AbstractHighlighter;
using KSyntaxHighlighting::Format;

static QString toHtmlRgbaString(const QColor &color)
{
    if (color.alpha() == 0xFF)
        return color.name();

    QString rgba = QStringLiteral("rgba(");
    rgba.append(QString::number(color.red()));
    rgba.append(QLatin1Char(','));
    rgba.append(QString::number(color.green()));
    rgba.append(QLatin1Char(','));
    rgba.append(QString::number(color.blue()));
    rgba.append(QLatin1Char(','));
    // this must be alphaF
    rgba.append(QString::number(color.alphaF()));
    rgba.append(QLatin1Char(')'));
    return rgba;
}

class HtmlHl : public AbstractHighlighter
{
public:
    HtmlHl()
        : out(&outputString)
    {
    }

    void setText(const QString &txt)
    {
        text = txt;
        QTextStream in(&text);

        out.reset();
        outputString.clear();

        bool inCodeBlock = false;

        KSyntaxHighlighting::State state;
        bool li = false;
        // World's smallest markdown parser :)
        while (!in.atEnd()) {
            currentLine = in.readLine();

            // allow empty lines in code blocks, no ruler here
            if (!inCodeBlock && currentLine.isEmpty()) {
                out << "<hr>";
                continue;
            }

            // list
            if (!li && currentLine.startsWith(QLatin1String("- "))) {
                currentLine.remove(0, 2);
                out << "<ul><li>";
                li = true;
            } else if (li && currentLine.startsWith(QLatin1String("- "))) {
                currentLine.remove(0, 2);
                out << "<li>";
            } else if (li) {
                out << "</li></ul>";
                li = false;
            }

            // code block
            if (!inCodeBlock && currentLine.startsWith(QLatin1String("```"))) {
                inCodeBlock = true;
                continue;
            } else if (inCodeBlock && currentLine.startsWith(QLatin1String("```"))) {
                inCodeBlock = false;
                continue;
            }

            // ATX heading
            if (currentLine.startsWith(QStringLiteral("# "))) {
                currentLine.remove(0, 2);
                currentLine = QStringLiteral("<h3>") + currentLine + QStringLiteral("</h3>");
                out << currentLine;
                continue;
            }

            state = highlightLine(currentLine, state);
            if (li) {
                out << "</li>";
                continue;
            }
            out << "\n<br>";
        }
    }

    QString html() const
    {
        //        while (!out.atEnd())
        //            qWarning() << out.readLine();
        return outputString;
    }

protected:
    void applyFormat(int offset, int length, const Format &format) override
    {
        if (!length)
            return;

        QString formatOutput;

        if (format.hasTextColor(theme())) {
            formatOutput = toHtmlRgbaString(format.textColor(theme()));
        }

        if (!formatOutput.isEmpty()) {
            out << "<span style=\"color:" << formatOutput << "\">";
        }

        out << currentLine.mid(offset, length).toHtmlEscaped();

        if (!formatOutput.isEmpty()) {
            out << "</span>";
        }
    }

private:
    QString text;
    QString currentLine;
    QString outputString;
    QTextStream out;
};

class Tooltip : public QTextBrowser
{
    Q_OBJECT

public:
    static Tooltip *self()
    {
        static Tooltip instance;
        return &instance;
    }

    void setTooltipText(const QString &text)
    {
        if (text.isEmpty())
            return;

        hl.setText(text);
        resizeTip(text);
        setHtml(hl.html());
    }

    void setView(KTextEditor::View *view)
    {
        // view changed?
        // => update definition
        // => update font
        if (view != m_view) {
            if (m_view) {
                m_view->focusProxy()->removeEventFilter(this);
            }

            m_view = view;
            hl.setDefinition(r.definitionForFileName(view->document()->url().toString()));
            updateFont();

            m_view->focusProxy()->installEventFilter(this);
        }
    }

    Tooltip(QWidget *parent = nullptr)
        : QTextBrowser(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget | Qt::ToolTip);
        document()->setDocumentMargin(2);
        setFrameStyle(QFrame::Box);
        connect(&m_hideTimer, &QTimer::timeout, this, &Tooltip::hideTooltip);

        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto updateColors = [this](KTextEditor::Editor *e) {
            auto theme = e->theme();
            hl.setTheme(theme);

            auto pal = palette();
            pal.setColor(QPalette::Base, theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
            setPalette(pal);

            updateFont();
        };
        updateColors(KTextEditor::Editor::instance());
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            hideTooltip();
            break;
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            if (!rect().contains(static_cast<QMouseEvent *>(e)->pos())) {
                hideTooltip();
            }
        default:
            break;
        }
        return false;
    }

    void updateFont()
    {
        if (!m_view)
            return;
        auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_view);
        auto font = ciface->configValue(QStringLiteral("font")).value<QFont>();
        setFont(font);
    }

    Q_SLOT void hideTooltip()
    {
        close();
        setText(QString());
    }

    void resizeTip(const QString &text)
    {
        QFontMetrics fm(font());
        QSize size = fm.size(0, text);
        size.setHeight(std::min(size.height(), m_view->height() / 3));
        size.setWidth(std::min(size.width(), m_view->width() / 2));
        resize(size);
    }

    void place(QPoint p)
    {
        if (p.x() + width() > m_view->x() + m_view->width())
            p.rx() -= 4 + width();
        if (p.y() + this->height() > m_view->y() + m_view->height())
            p.ry() -= 24 + this->height();
        if (p.y() < m_view->y())
            p.setY(m_view->y());
        if (p.x() + this->width() > m_view->x() + m_view->width())
            p.setX(m_view->x() + m_view->width() - this->width());
        if (p.x() < m_view->x())
            p.setX(m_view->x());
        if (p.y() + this->height() > m_view->y() + m_view->height())
            p.setY(m_view->y() + m_view->height() - this->height());
        this->move(p);
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        m_hideTimer.start(1000);
        return QTextBrowser::showEvent(event);
    }

    void enterEvent(QEvent *event) override
    {
        m_hideTimer.stop();
        return QTextBrowser::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (!m_hideTimer.isActive()) {
            hideTooltip();
        }
        return QTextBrowser::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        auto pos = event->pos();
        if (rect().contains(pos)) {
            return QTextBrowser::mouseMoveEvent(event);
        }
        hideTooltip();
    }

private:
    KTextEditor::View *m_view;
    QTimer m_hideTimer;
    HtmlHl hl;
    KSyntaxHighlighting::Repository r;
};

void LspTooltip::show(const QString &text, QPoint pos, KTextEditor::View *v)
{
    if (text.isEmpty())
        return;

    Tooltip::self()->setView(v);
    Tooltip::self()->setTooltipText(text);
    Tooltip::self()->place(pos);
    Tooltip::self()->show();
}

#include "lsptooltip.moc"
