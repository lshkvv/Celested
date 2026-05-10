#include "detectionrectitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCursor>

DetectionRectItem::DetectionRectItem(int detectionId, const QRectF &rect, QGraphicsItem *parent)
    : QObject()
    , QGraphicsRectItem(rect, parent)
    , m_detectionId(detectionId)
    , m_dragMode(NoDrag)
    , m_handleSize(10.0)
    , m_minSize(12.0)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptHoverEvents(true);
}

int DetectionRectItem::detectionId() const
{
    return m_detectionId;
}

QRectF DetectionRectItem::handleRect() const
{
    QRectF r = rect();
    return QRectF(r.right() - m_handleSize,
                  r.bottom() - m_handleSize,
                  m_handleSize,
                  m_handleSize);
}

void DetectionRectItem::updateCursor(const QPointF &localPos)
{
    if (handleRect().contains(localPos))
        setCursor(Qt::SizeFDiagCursor);
    else
        setCursor(Qt::SizeAllCursor);
}

QVariant DetectionRectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsRectItem::itemChange(change, value);
}

void DetectionRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragStartPos = pos();
    m_initialRect = rect();

    if (handleRect().contains(event->pos())) {
        m_dragMode = ResizeBottomRight;
        setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        m_dragMode = MoveDrag;
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    emit clicked(m_detectionId);
    QGraphicsRectItem::mousePressEvent(event);
}

void DetectionRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragMode == ResizeBottomRight) {
        QRectF r = m_initialRect;
        qreal newWidth = event->pos().x() - r.left();
        qreal newHeight = event->pos().y() - r.top();

        if (newWidth < m_minSize)
            newWidth = m_minSize;
        if (newHeight < m_minSize)
            newHeight = m_minSize;

        prepareGeometryChange();
        setRect(QRectF(r.left(), r.top(), newWidth, newHeight));
        update();
        event->accept();
        return;
    }

    QGraphicsRectItem::mouseMoveEvent(event);
}

void DetectionRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsRectItem::mouseReleaseEvent(event);

    setFlag(QGraphicsItem::ItemIsMovable, true);

    const QRectF sceneRect = mapRectToScene(rect());
    emit geometryChanged(m_detectionId, sceneRect);
    emit clicked(m_detectionId);

    m_dragMode = NoDrag;
}

void DetectionRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    updateCursor(event->pos());
    QGraphicsRectItem::hoverMoveEvent(event);
}

void DetectionRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    if (isSelected()) {
        const QRectF hr = handleRect();
        painter->setPen(QPen(QColor(255, 255, 255), 1));
        painter->setBrush(QBrush(QColor(255, 170, 0)));
        painter->drawRect(hr);
    }

    Q_UNUSED(widget)
    Q_UNUSED(option)
}
