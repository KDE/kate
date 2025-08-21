#pragma once

#include <QColor>
#include <QTimeLine>
#include <QWidget>

/**
 * This class is used to create a colored status overlay to tool-view tab buttons in Kate
 * @note When a tab button is moved, the tab-button is destroyed and a new button is created.
 *       For that reason, it is recommended to store a QPointer to the StatusOverlay, to not risk
 *       using a dangling pointer.
 */
class StatusOverlay : public QWidget
{
public:
    enum class Type {
        None,
        Neutral,
        Positive,
        Warning,
        Error,
    };
    Q_ENUM(Type)

    explicit StatusOverlay(QWidget *parent);
    void setType(Type type);
    void setGlowing(bool glowing);
    /**
     * Set progress
     * @param progressRatio is a value between 0.0 and 1.0
     */
    void setProgress(double progressRatio);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Type m_type = Type::None;
    QColor m_color;
    QWidget *m_tabButton = nullptr;
    QTimeLine m_timeline;
    double m_progress = 0.0;
};
