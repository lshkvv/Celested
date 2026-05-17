#include "imageview.h"

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRubberBand>
#include <QWheelEvent>

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_drawModeEnabled(false)
    , m_drawing(false)
    , m_temporaryPanActive(false)
    , m_rubberBand(new QRubberBand(QRubberBand::Rectangle, this))
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_rubberBand->hide();
}

void ImageView::setDrawModeEnabled(bool enabled)
{
    m_drawModeEnabled = enabled;

    if (!enabled) {
        m_drawing = false;
        m_rubberBand->hide();
    }

    if (!m_temporaryPanActive)
        setDragMode(enabled ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
}

bool ImageView::drawModeEnabled() const
{
    return m_drawModeEnabled;
}

void ImageView::zoomIn()
{
    scale(1.15, 1.15);
}

void ImageView::zoomOut()
{
    scale(1.0 / 1.15, 1.0 / 1.15);
}

void ImageView::fitSceneInView()
{
    if (!scene() || scene()->sceneRect().isEmpty())
        return;

    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (!scene() || scene()->sceneRect().isEmpty()) {
        event->ignore();
        return;
    }

    const qreal currentScale = transform().m11();
    constexpr qreal kMinScale = 0.05;
    constexpr qreal kMaxScale = 32.0;

    if (event->angleDelta().y() > 0) {
        if (currentScale < kMaxScale)
            zoomIn();
    } else {
        if (currentScale > kMinScale)
            zoomOut();
    }

    event->accept();
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (m_temporaryPanActive) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (m_drawModeEnabled && event->button() == Qt::LeftButton) {
        m_drawing = true;
        m_origin = event->pos();
        m_rubberBand->setGeometry(QRect(m_origin, QSize()));
        m_rubberBand->show();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drawing) {
        m_rubberBand->setGeometry(QRect(m_origin, event->pos()).normalized());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_drawing && event->button() == Qt::LeftButton) {
        m_drawing = false;
        m_rubberBand->hide();

        const QRectF rect = currentSceneRectFromRubberBand();
        if (rect.isValid() && rect.width() > 3.0 && rect.height() > 3.0)
            emit detectionDrawn(rect.normalized());

        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void ImageView::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        setTemporaryPanEnabled(true);
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void ImageView::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
        setTemporaryPanEnabled(false);
        event->accept();
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

QRectF ImageView::currentSceneRectFromRubberBand() const
{
    if (!scene())
        return QRectF();

    const QRect viewRect = m_rubberBand->geometry();
    const QPointF topLeft = mapToScene(viewRect.topLeft());
    const QPointF bottomRight = mapToScene(viewRect.bottomRight());

    return QRectF(topLeft, bottomRight).normalized();
}

void ImageView::setTemporaryPanEnabled(bool enabled)
{
    if (m_temporaryPanActive == enabled)
        return;

    m_temporaryPanActive = enabled;

    if (enabled) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        viewport()->setCursor(Qt::OpenHandCursor);
    } else {
        setDragMode(m_drawModeEnabled ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
        viewport()->unsetCursor();
    }

    emit temporaryPanStateChanged(enabled);
}
