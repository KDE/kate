/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "rainbowparens_plugin.h"

#include <QGradient>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QScopeGuard>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Document>
#include <KTextEditor/View>

K_PLUGIN_FACTORY_WITH_JSON(RainbowParenPluginFactory, "rainbowparens_plugin.json", registerPlugin<RainbowParenPlugin>();)

RainbowParenPlugin::RainbowParenPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
    readConfig();
}

KTextEditor::ConfigPage *RainbowParenPlugin::configPage(int number, QWidget *parent)
{
    if (number == 0) {
        return new RainbowParenConfigPage(parent, this);
    }
    return nullptr;
}

void RainbowParenPlugin::readConfig()
{
    if (m_attrs.empty()) {
        m_attrs.resize(5);
        for (auto &attr : m_attrs) {
            attr = new KTextEditor::Attribute;
        }
    }

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ColoredBrackets"));
    QColor br = config.readEntry("color1", QStringLiteral("#1275ef"));
    m_attrs[0]->setForeground(br);
    br = config.readEntry("color2", QStringLiteral("#f83c1f"));
    m_attrs[1]->setForeground(br);
    br = config.readEntry("color3", QStringLiteral("#9dba1e"));
    m_attrs[2]->setForeground(br);
    br = config.readEntry("color4", QStringLiteral("#e219e2"));
    m_attrs[3]->setForeground(br);
    br = config.readEntry("color5", QStringLiteral("#37d21c"));
    m_attrs[4]->setForeground(br);
}

QObject *RainbowParenPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new RainbowParenPluginView(this, mainWindow);
}

RainbowParenPluginView::RainbowParenPluginView(RainbowParenPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(plugin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
{
    connect(mainWin, &KTextEditor::MainWindow::viewChanged, this, &RainbowParenPluginView::viewChanged);
    QTimer::singleShot(50, this, [this] {
        if (auto *view = m_mainWindow->activeView()) {
            m_lastUserColor = rehighlight(view, m_ranges, m_lastUserColor, m_plugin->colorsList());
        }
    });
    m_rehighlightTimer.setInterval(200);
    m_rehighlightTimer.setSingleShot(true);
    m_rehighlightTimer.callOnTimeout(this, [this] {
        if (m_activeView) {
            m_lastUserColor = rehighlight(m_activeView, m_ranges, m_lastUserColor, m_plugin->colorsList());
        }
    });
}

static void getSavedRangesForDoc(std::vector<RainbowParenPluginView::SavedRanges> &ranges,
                                 std::vector<std::unique_ptr<KTextEditor::MovingRange>> &outRanges,
                                 KTextEditor::Document *d)
{
    auto it = std::find_if(ranges.begin(), ranges.end(), [d](const RainbowParenPluginView::SavedRanges &r) {
        return r.doc == d;
    });
    if (it != ranges.end()) {
        outRanges = std::move(it->ranges);
        ranges.erase(it);
    }
}

void RainbowParenPluginView::viewChanged(KTextEditor::View *view)
{
    if (!view) {
        return;
    }

    // disconnect and clear previous doc stuff
    if (m_activeView) {
        disconnect(m_activeView, &KTextEditor::View::verticalScrollPositionChanged, this, &RainbowParenPluginView::onScrollChanged);

        auto doc = m_activeView->document();
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
        disconnect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &RainbowParenPluginView::clearRanges);
#endif
        disconnect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &RainbowParenPluginView::clearRanges);
        disconnect(doc, &KTextEditor::Document::textInserted, this, &RainbowParenPluginView::onTextInserted);
        disconnect(doc, &KTextEditor::Document::textRemoved, this, &RainbowParenPluginView::onTextRemoved);

        // Remove if we already have this view's ranges saved
        clearSavedRangesForDoc(doc);

        // save these ranges so that if we are in a split view etc, we don't
        // loose the bracket coloring
        SavedRanges saved;
        saved.doc = doc;
        saved.ranges = std::move(m_ranges);
        m_savedRanges.push_back(std::move(saved));
        if (m_savedRanges.size() > 4) {
            m_savedRanges.erase(m_savedRanges.begin());
        }

        // clear ranges for this doc if it gets closed
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
        connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &RainbowParenPluginView::clearSavedRangesForDoc, Qt::UniqueConnection);
#endif
        connect(doc,
                &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent,
                this,
                &RainbowParenPluginView::clearSavedRangesForDoc,
                Qt::UniqueConnection);
    }

    m_ranges.clear();
    m_activeView = view;

    // get any existing ranges for this view
    getSavedRangesForDoc(m_savedRanges, m_ranges, m_activeView->document());

    connect(view, &KTextEditor::View::verticalScrollPositionChanged, this, &RainbowParenPluginView::onScrollChanged, Qt::UniqueConnection);

    auto doc = m_activeView->document();
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
    connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &RainbowParenPluginView::clearRanges, Qt::UniqueConnection);
#endif
    connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &RainbowParenPluginView::clearRanges, Qt::UniqueConnection);
    connect(doc, &KTextEditor::Document::textInserted, this, &RainbowParenPluginView::onTextInserted, Qt::UniqueConnection);
    connect(doc, &KTextEditor::Document::textRemoved, this, &RainbowParenPluginView::onTextRemoved, Qt::UniqueConnection);

    m_lastUserColor = rehighlight(m_activeView, m_ranges, m_lastUserColor, m_plugin->colorsList());
}

static void onTextChanged(RainbowParenPluginView *p, const QString &text)
{
    auto isBracket = [](QChar c) {
        return c == QLatin1Char('{') || c == QLatin1Char('(') || c == QLatin1Char(')') || c == QLatin1Char('}') || c == QLatin1Char('[')
            || c == QLatin1Char(']');
    };
    if (text.size() > 100) {
        p->requestRehighlight();
        return;
    }

    for (auto c : text) {
        if (isBracket(c)) {
            p->requestRehighlight();
            break;
        }
    }
}

void RainbowParenPluginView::onTextInserted(KTextEditor::Document *, KTextEditor::Cursor, const QString &text)
{
    onTextChanged(this, text);
}

void RainbowParenPluginView::onTextRemoved(KTextEditor::Document *, KTextEditor::Range, const QString &text)
{
    onTextChanged(this, text);
}

void RainbowParenPluginView::onScrollChanged()
{
    requestRehighlight(50);
}

void RainbowParenPluginView::requestRehighlight(int delay)
{
    if (!m_rehighlightTimer.isActive()) {
        m_rehighlightTimer.start(delay);
    }
}

void RainbowParenPluginView::clearRanges(KTextEditor::Document *)
{
    m_ranges.clear();
}

void RainbowParenPluginView::clearSavedRangesForDoc(KTextEditor::Document *doc)
{
    auto it = std::find_if(m_savedRanges.begin(), m_savedRanges.end(), [doc](const RainbowParenPluginView::SavedRanges &r) {
        return r.doc == doc;
    });
    if (it != m_savedRanges.end()) {
        m_savedRanges.erase(it);
    }
}

static bool isCommentOrString(KTextEditor::Document *doc, int line, int col)
{
    const auto style = doc->defaultStyleAt({line, col});
    return style == KSyntaxHighlighting::Theme::TextStyle::Comment || style == KSyntaxHighlighting::Theme::TextStyle::Char
        || style == KSyntaxHighlighting::Theme::TextStyle::String;
}

using ColoredBracket = std::unique_ptr<KTextEditor::MovingRange>;

struct ColoredBracketPair {
    std::unique_ptr<KTextEditor::MovingRange> start;
    std::unique_ptr<KTextEditor::MovingRange> end;
};

/**
 * Helper function to find if we have @p open and @p close already in oldRanges so we
 * can reuse it
 */
static ColoredBracketPair existingColoredBracketForPos(std::vector<ColoredBracket> &oldRanges, KTextEditor::Cursor open, KTextEditor::Cursor close)
{
    bool openFound = false;
    bool closeFound = false;
    int sIdx = 0;
    int eIdx = 0;
    int i = 0;
    for (const auto &range : oldRanges) {
        if (!openFound && range->start() == open) {
            openFound = true;
            sIdx = i;
        } else if (!closeFound && range->start() == close) {
            closeFound = true;
            eIdx = i;
        }
        if (openFound && closeFound) {
            ColoredBracketPair ret = {std::move(oldRanges[sIdx]), std::move(oldRanges[eIdx])};
            oldRanges.erase(std::remove(oldRanges.begin(), oldRanges.end(), nullptr), oldRanges.end());
            return ret;
        }
        i++;
    }
    return {nullptr, nullptr};
}

/**
 * This function contains the entirety of the algorithm
 *
 * How it works (or supposed to):
 *  - Get current view line range (our viewport)
 *  - Collect all bracket pairs in it
 *  - Color them
 *
 * We check for vertical scrolling + text insertion to redo everything.
 * This allows us to do a lot less work and still be able to do coloring
 * of brackets.
 */
size_t rehighlight(KTextEditor::View *view,
                   std::vector<std::unique_ptr<KTextEditor::MovingRange>> &ranges,
                   size_t lastUserColorIdx,
                   const std::vector<KTextEditor::Attribute::Ptr> &attrs)
{
    // we only care about lines that are in the viewport
    int start = view->firstDisplayedLine();
    int end = view->lastDisplayedLine();
    if (end < start) {
        qWarning("RainbowParenPluginView: Unexpected end < start");
        return lastUserColorIdx;
    }

    // if the lines are folded we can get really big range
    if (end - start > 800) {
        end = start + 800;
    }

    // Check if @p c matches any opener
    auto isBracketOpen = [](QChar c) {
        return c == u'{' || c == u'[' || c == u'(';
    };
    // Check if @p is a bracket closer for @p opener
    auto matchesCloser = [](QChar in, QChar opener) {
        return (opener == u'{' && in == u'}') || (opener == u'[' && in == u']') || (opener == u'(' && in == u')');
    };
    auto createOneColumnRange = [](KTextEditor::Cursor c) {
        return KTextEditor::Range(c.line(), c.column(), c.line(), c.column() + 1);
    };

    // A struct representing an opening bracket
    struct Opener {
        QChar bracketChar;
        KTextEditor::Cursor pos;
    };

    // A struct representing a bracket pair (open, close)
    struct BracketPair {
        KTextEditor::Cursor opener;
        KTextEditor::Cursor closer;
    };

    // contains all final bracket pairs of current viewport
    std::vector<BracketPair> parens;
    // the stack we use to build "parens"
    std::vector<Opener> bracketStack;

    KTextEditor::Document *doc = view->document();

    for (int l = start; l <= end; ++l) {
        const QString line = doc->line(l);
        for (int c = 0; c < line.length(); ++c) {
            if (isCommentOrString(doc, l, c)) {
                continue;
            }

            if (isBracketOpen(line.at(c))) {
                bracketStack.push_back(Opener{
                    .bracketChar = line.at(c),
                    .pos = {l, c},
                });
            } else if (!bracketStack.empty() && matchesCloser(line.at(c), bracketStack.back().bracketChar)) {
                auto openerPos = bracketStack.back().pos;
                bracketStack.pop_back();
                // no '()' or '{}' highlighting
                if (openerPos.line() == l && openerPos.column() + 1 == c) {
                    continue;
                }
                parens.push_back({openerPos, {l, c}});
            }
        }
    }

    // We reuse ranges completely if there was no change but the user
    // was only scrolling. This allows the colors to stay somewhat stable
    // and not change all the time
    auto oldRanges = std::move(ranges);

    if (parens.empty())
        return lastUserColorIdx;

    // sort by start paren
    // Necessary so that we can get alternating colors for brackets
    // on the same line
    std::stable_sort(parens.begin(), parens.end(), [](const auto &a, const auto &b) {
        return a.opener < b.opener;
    });

    auto onSameLine = [](KTextEditor::Cursor open, KTextEditor::Cursor close) {
        return open.line() == close.line();
    };

    size_t idx = 0;
    size_t color = lastUserColorIdx;
    int lastParenLine = 0;
    for (auto p : parens) {
        // scope guard to ensure we always update stuff for every iteration
        auto updater = qScopeGuard([&idx, &lastParenLine, p] {
            idx++;
            lastParenLine = p.opener.line();
        });

        // open/close brackets on same line?
        if (lastParenLine != p.opener.line() && onSameLine(p.opener, p.closer)) {
            // check next position,
            // if the next opener's line is not the same as current opener's line
            // then it is on some other line which means we don't need to highlight
            // this pair of brackets
            if (idx + 1 < parens.size() && parens.at(idx + 1).opener.line() != p.opener.line()) {
                continue;
            }
        }

        // find if we already highlighted this bracket
        auto [existingStart, existingEnd] = existingColoredBracketForPos(oldRanges, p.opener, p.closer);
        if (existingStart && existingEnd) {
            // ensure start and end have same attribute and expected range
            KTextEditor::Range expectedStart = createOneColumnRange(p.opener);
            if (existingStart->toRange() != expectedStart) {
                existingStart->setRange(expectedStart, existingEnd->attribute());
            } else if (existingStart->attribute() != existingEnd->attribute()) {
                existingStart->setAttribute(existingEnd->attribute());
            }

            KTextEditor::Range expectedEnd = createOneColumnRange(p.closer);
            if (existingEnd->toRange() != expectedEnd) {
                existingEnd->setRange(expectedEnd);
            }

            ranges.push_back(std::move(existingStart));
            ranges.push_back(std::move(existingEnd));
            auto attrib = ranges.back()->attribute();
            auto it = std::find(attrs.begin(), attrs.end(), attrib);
            auto prevColor = color;
            color = std::distance(attrs.begin(), it) + 1;
            color = prevColor == color ? color + 1 : color;
            continue;
        }

        std::unique_ptr<KTextEditor::MovingRange> r(doc->newMovingRange(createOneColumnRange(p.opener)));
        r->setAttribute(attrs[color % numberOfColors]);

        std::unique_ptr<KTextEditor::MovingRange> r2(doc->newMovingRange(createOneColumnRange(p.closer)));
        r2->setAttribute(attrs[color % numberOfColors]);

        ranges.push_back(std::move(r));
        ranges.push_back(std::move(r2));

        color++;
    }

    oldRanges.clear();
    return color;
}

RainbowParenConfigPage::RainbowParenConfigPage(QWidget *parent, RainbowParenPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    auto layout = new QVBoxLayout(this);

    auto gb = new QGroupBox(this);
    gb->setTitle(i18nc("group title", "Bracket Colors"));
    auto gbLayout = new QVBoxLayout(gb);

    for (auto &btn : m_btns) {
        gbLayout->addWidget(&btn);
        btn.setMaximumWidth(150);
        connect(&btn, &KColorButton::changed, this, &RainbowParenConfigPage::changed);
    }

    layout->addWidget(gb);
    layout->addStretch();

    reset();
}

QString RainbowParenConfigPage::name() const
{
    return i18n("Colored Brackets");
}

QString RainbowParenConfigPage::fullName() const
{
    return i18n("Colored Brackets Settings");
}

static QIcon ColoredBracketsIcon(const QWidget *_this)
{
    QRect r(QPoint(0, 0), _this->devicePixelRatioF() * QSize(16, 16));
    QPixmap pix(r.size());
    pix.fill(Qt::transparent);
    QPainter paint(&pix);
    if (paint.fontMetrics().height() > r.height()) {
        auto f = paint.font();
        f.setPixelSize(14);
        paint.setFont(f);
    }
    paint.setPen(QPen(QBrush(QGradient(QGradient::Preset::PhoenixStart)), 3));
    paint.drawText(r, Qt::AlignCenter, QStringLiteral("{.}"));
    paint.end();
    return pix;
}

QIcon RainbowParenConfigPage::icon() const
{
    if (m_icon.isNull()) {
        const_cast<RainbowParenConfigPage *>(this)->m_icon = ColoredBracketsIcon(this);
    }
    return m_icon;
}

void RainbowParenConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ColoredBrackets"));
    config.writeEntry("color1", m_btns[0].color().name(QColor::HexRgb));
    config.writeEntry("color2", m_btns[1].color().name(QColor::HexRgb));
    config.writeEntry("color3", m_btns[2].color().name(QColor::HexRgb));
    config.writeEntry("color4", m_btns[3].color().name(QColor::HexRgb));
    config.writeEntry("color5", m_btns[4].color().name(QColor::HexRgb));
    config.sync();
    m_plugin->readConfig();
}

void RainbowParenConfigPage::reset()
{
    int i = 0;
    for (const auto &attr : m_plugin->colorsList()) {
        m_btns[i++].setColor(attr->foreground().color());
    }
}

void RainbowParenConfigPage::defaults()
{
}

#include "rainbowparens_plugin.moc"
