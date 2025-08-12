#include "TabOverlay.h"

#include <KColorScheme>
#include <QPainter>

TabOverlay::TabOverlay(QWidget *parent)
    : QWidget(parent)
    , m_tabButton(parent)
    , m_timeline(1500, this)
{
    if (!parent) {
        hide();
        return;
    }
    m_timeline.setDirection(QTimeLine::Forward);
    m_timeline.setEasingCurve(QEasingCurve::SineCurve);
    m_timeline.setFrameRange(50, 190);
    m_timeline.setLoopCount(0);
    auto update = QOverload<>::of(&QWidget::update);
    connect(&m_timeline, &QTimeLine::valueChanged, this, update);

    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setGeometry(parent->geometry());
    move({0, 0});
    show();
    raise();
}

void TabOverlay::setType(TabOverlay::Type type)
{
    if (!m_tabButton) {
        return;
    }
    m_type = type;
    if (m_tabButton->size() != size()) {
        resize(m_tabButton->size());
    }

    switch (m_type) {
    case Type::None:
        break;
    case Type::Neutral:
        m_color = KColorScheme().foreground(KColorScheme::LinkText).color();
        break;
    case Type::Positive:
        m_color = KColorScheme().foreground(KColorScheme::PositiveText).color();
        break;
    case Type::Warning:
        m_color = KColorScheme().foreground(KColorScheme::NeutralText).color();
        break;
    case Type::Error:
        m_color = KColorScheme().foreground(KColorScheme::NegativeText).color();
        break;
    }
    update();
}

void TabOverlay::setGlowing(bool glowing)
{
    if (!glowing) {
        m_timeline.stop();
    } else if (m_timeline.state() != QTimeLine::Running) {
        m_timeline.start();
    }
}

void TabOverlay::setProgress(double progressRatio)
{
    m_progress = std::min(1.0, progressRatio);
    update();
}

void TabOverlay::paintEvent(QPaintEvent *)
{
    if (!m_tabButton || m_type == Type::None) {
        return;
    }

    QPainter p(this);
    p.setOpacity(0.25);
    p.setPen(Qt::NoPen);

    int alpha = m_timeline.state() == QTimeLine::Running ? m_timeline.currentFrame() : 255;
    if (m_progress > 0.0001) {
        QColor bgColor = KColorScheme().foreground(KColorScheme::LinkText).color();
        int overlayWidth = rect().width() - 2;
        int progressMargin = overlayWidth - std::round(overlayWidth * m_progress);

        bgColor.setAlpha(alpha * 0.3);
        p.setBrush(bgColor);
        p.drawRect(rect().adjusted(1 + (overlayWidth - progressMargin), 1, -1, -1));

        m_color.setAlpha(alpha + 60);
        p.setBrush(m_color);
        p.drawRect(rect().adjusted(1, 1, -progressMargin - 1, -1));
    } else {
        m_color.setAlpha(alpha);
        p.setBrush(m_color);
        p.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}
