#include "imageview.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QResizeEvent>
#include <QPainter>

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_drawModeEnabled(false)
    , m_drawing(false)
    , m_fitMode(true)
    , m_previewRect(nullptr)
{
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void ImageView::setDrawModeEnabled(bool enabled)
{
    m_drawModeEnabled = enabled;

    if (enabled)
        setDragMode(QGraphicsView::NoDrag);
    else
        setDragMode(QGraphicsView::ScrollHandDrag);

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

void ImageView::zoomIn()
{
    applyZoom(1.15);
}

void ImageView::zoomOut()
{
    applyZoom(1.0 / 1.15);
}

void ImageView::fitSceneInView()
{
    if (!scene() || scene()->sceneRect().isEmpty())
        return;

    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    m_fitMode = true;
}

void ImageView::applyZoom(qreal factor)
{
    m_fitMode = false;
    scale(factor, factor);
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

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (!scene()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    if (event->angleDelta().y() > 0)
        applyZoom(1.15);
    else
        applyZoom(1.0 / 1.15);

    event->accept();
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    if (m_fitMode)
        fitSceneInView();
}
