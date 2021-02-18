/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "lsptooltip.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QScreen>
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
    void setTooltipText(const QString &text)
    {
        if (text.isEmpty())
            return;

        hl.setText(text);
        setHtml(hl.html());
        resizeTip(text);
    }

    void setView(KTextEditor::View *view)
    {
        // view changed?
        // => update definition
        // => update font
        if (view != m_view) {
            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->removeEventFilter(this);
            }

            m_view = view;

            hl.setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(m_view->document()->url().toString()));
            updateFont();

            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->installEventFilter(this);
            }
        }
    }

    Tooltip(QWidget *parent)
        : QTextBrowser(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget | Qt::ToolTip);
        setAttribute(Qt::WA_DeleteOnClose, true);
        document()->setDocumentMargin(5);
        setFrameStyle(QFrame::Box | QFrame::Raised);
        connect(&m_hideTimer, &QTimer::timeout, this, &Tooltip::hideTooltip);

        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto updateColors = [this](KTextEditor::Editor *e) {
            auto theme = e->theme();
            hl.setTheme(theme);

            auto pal = palette();
            const QColor bg = theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
            pal.setColor(QPalette::Base, bg);
            const QColor normal = theme.textColor(KSyntaxHighlighting::Theme::Normal);
            pal.setColor(QPalette::Text, normal);
            setPalette(pal);

            updateFont();
        };
        updateColors(KTextEditor::Editor::instance());
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    }

    bool eventFilter(QObject *, QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            hideTooltip();
            break;
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::FocusOut:
        case QEvent::FocusIn:
            if (!inContextMenu)
                hideTooltip();
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            if (!rect().contains(static_cast<QMouseEvent *>(e)->pos())) {
                hideTooltip();
            }
            break;
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
        deleteLater();
    }

    void resizeTip(const QString &text)
    {
        QFontMetrics fm(font());
        QSize size = fm.size(0, text);

        // make sure we have the correct height
        // size above gives us correct width but not
        // correct height
        qreal totalHeight = document()->size().height();
        // add +1 line height to prevent scrollbar from appearing with small
        // tooltips
        int lineHeight = totalHeight / document()->lineCount();
        const int height = totalHeight + lineHeight;

        size.setHeight(std::min(height, m_view->height() / 3));
        size.setWidth(std::min(size.width(), m_view->width() / 2));
        resize(size);
    }

    void place(QPoint p)
    {
        QRect screen = QApplication::screenAt(p)->availableGeometry();

        if (p.x() + width() > screen.x() + screen.width())
            p.rx() -= 4 + width();
        if (p.y() + this->height() > screen.y() + screen.height())
            p.ry() -= 24 + this->height();
        if (p.y() < screen.y())
            p.setY(screen.y());
        if (p.x() + this->width() > screen.x() + screen.width())
            p.setX(screen.x() + screen.width() - this->width());
        if (p.x() < screen.x())
            p.setX(screen.x());
        if (p.y() + this->height() > screen.y() + screen.height())
            p.setY(screen.y() + screen.height() - this->height());

        this->move(p);
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        m_hideTimer.start(1000);
        QTextBrowser::showEvent(event);
    }

    void enterEvent(QEvent *event) override
    {
        inContextMenu = false;
        m_hideTimer.stop();
        QTextBrowser::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (!m_hideTimer.isActive() && !inContextMenu) {
            hideTooltip();
        }
        QTextBrowser::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        auto pos = event->pos();
        if (rect().contains(pos)) {
            return QTextBrowser::mouseMoveEvent(event);
        }
        hideTooltip();
    }

    void contextMenuEvent(QContextMenuEvent *e) override
    {
        inContextMenu = true;
        QTextBrowser::contextMenuEvent(e);
    }

private:
    bool inContextMenu = false;
    QPointer<KTextEditor::View> m_view;
    QTimer m_hideTimer;
    HtmlHl hl;
};

void LspTooltip::show(const QString &text, QPoint pos, KTextEditor::View *v)
{
    if (text.isEmpty())
        return;

    if (!v || !v->document()) {
        return;
    }

    auto tooltip = new Tooltip(v);
    tooltip->show();
    tooltip->setView(v);
    tooltip->setTooltipText(text);
    tooltip->place(pos);
}

#include "lsptooltip.moc"
