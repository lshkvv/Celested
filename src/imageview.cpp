#include "imageview.h"

#include <QMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QColor>

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_drawModeEnabled(false)
    , m_drawing(false)
    , m_previewRect(nullptr)
{
    setMouseTracking(true);
}

void ImageView::setDrawModeEnabled(bool enabled)
{
    m_drawModeEnabled = enabled;

    if (!enabled && m_previewRect) {
        if (scene())
            scene()->removeItem(m_previewRect);
        delete m_previewRect;
        m_previewRect = nullptr;
        m_drawing = false;
    }
}

bool ImageView::isDrawModeEnabled() const
{
    return m_drawModeEnabled;
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (!m_drawModeEnabled || !scene() || event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    m_drawing = true;
    m_startScenePos = mapToScene(event->pos());

    if (m_previewRect) {
        scene()->removeItem(m_previewRect);
        delete m_previewRect;
        m_previewRect = nullptr;
    }

    QPen pen(QColor(255, 80, 80));
    pen.setWidth(2);
    QBrush brush(QColor(255, 80, 80, 40));

    m_previewRect = scene()->addRect(QRectF(m_startScenePos, m_startScenePos), pen, brush);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_drawModeEnabled || !m_drawing || !m_previewRect || !scene()) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    const QPointF currentScenePos = mapToScene(event->pos());
    const QRectF rect = QRectF(m_startScenePos, currentScenePos).normalized();
    m_previewRect->setRect(rect);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_drawModeEnabled || !m_drawing || !m_previewRect || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    const QRectF rect = m_previewRect->rect().normalized();

    if (scene())
        scene()->removeItem(m_previewRect);
    delete m_previewRect;
    m_previewRect = nullptr;
    m_drawing = false;

    if (rect.width() > 3.0 && rect.height() > 3.0)
        emit detectionDrawn(rect);
}
