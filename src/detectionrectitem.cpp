#include "detectionrectitem.h"

#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

DetectionRectItem::DetectionRectItem(int detectionId,
                                     const QRectF &rect,
                                     QGraphicsItem *parent)
    : QObject()
    , QGraphicsRectItem(rect.normalized(), parent)
    , m_detectionId(detectionId)
    , m_resizing(false)
    , m_activeHandle(NoHandle)
    , m_handleSize(8.0)
    , m_minSize(6.0)
{
    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

int DetectionRectItem::detectionId() const
{
    return m_detectionId;
}

QVariant DetectionRectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsRectItem::itemChange(change, value);
}

void DetectionRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit clicked(m_detectionId);

    m_activeHandle = handleAt(event->pos());
    m_pressRect = rect();

    if (m_activeHandle != NoHandle) {
        m_resizing = true;
        setFlag(QGraphicsItem::ItemIsMovable, false);
        event->accept();
        update();
        return;
    }

    m_resizing = false;
    setFlag(QGraphicsItem::ItemIsMovable, true);
    QGraphicsRectItem::mousePressEvent(event);
}

void DetectionRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizing) {
        setRect(resizedRect(event->scenePos()));
        update();
        event->accept();
        return;
    }

    QGraphicsRectItem::mouseMoveEvent(event);
}

void DetectionRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizing) {
        m_resizing = false;
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setRect(normalizedMinimumRect(rect()));
        emit geometryChanged(m_detectionId, sceneBoundingRect().normalized());
        update();
        event->accept();
        return;
    }

    const QRectF before = rect();
    QGraphicsRectItem::mouseReleaseEvent(event);
    const QRectF after = normalizedMinimumRect(rect());
    if (after != before) {
        setRect(after);
        emit geometryChanged(m_detectionId, sceneBoundingRect().normalized());
    }
}

void DetectionRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    updateCursor(event->pos());
    QGraphicsRectItem::hoverMoveEvent(event);
}

void DetectionRectItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget *widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 220));

    const QList<Handle> handles = {
        TopLeftHandle, TopHandle, TopRightHandle,
        LeftHandle, RightHandle,
        BottomLeftHandle, BottomHandle, BottomRightHandle
    };

    for (Handle h : handles)
        painter->drawRect(handleRect(h));

    painter->restore();
}

QRectF DetectionRectItem::handleRect(Handle handle) const
{
    const QRectF r = rect();
    const qreal hs = m_handleSize;
    const qreal x1 = r.left();
    const qreal x2 = r.center().x();
    const qreal x3 = r.right();
    const qreal y1 = r.top();
    const qreal y2 = r.center().y();
    const qreal y3 = r.bottom();

    switch (handle) {
    case TopLeftHandle:     return QRectF(x1 - hs / 2, y1 - hs / 2, hs, hs);
    case TopHandle:         return QRectF(x2 - hs / 2, y1 - hs / 2, hs, hs);
    case TopRightHandle:    return QRectF(x3 - hs / 2, y1 - hs / 2, hs, hs);
    case LeftHandle:        return QRectF(x1 - hs / 2, y2 - hs / 2, hs, hs);
    case RightHandle:       return QRectF(x3 - hs / 2, y2 - hs / 2, hs, hs);
    case BottomLeftHandle:  return QRectF(x1 - hs / 2, y3 - hs / 2, hs, hs);
    case BottomHandle:      return QRectF(x2 - hs / 2, y3 - hs / 2, hs, hs);
    case BottomRightHandle: return QRectF(x3 - hs / 2, y3 - hs / 2, hs, hs);
    default:                return QRectF();
    }
}

DetectionRectItem::Handle DetectionRectItem::handleAt(const QPointF &pos) const
{
    const QList<Handle> handles = {
        TopLeftHandle, TopHandle, TopRightHandle,
        LeftHandle, RightHandle,
        BottomLeftHandle, BottomHandle, BottomRightHandle
    };

    for (Handle h : handles) {
        if (handleRect(h).contains(pos))
            return h;
    }

    return NoHandle;
}

void DetectionRectItem::updateCursor(const QPointF &pos)
{
    switch (handleAt(pos)) {
    case TopLeftHandle:
    case BottomRightHandle:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRightHandle:
    case BottomLeftHandle:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case LeftHandle:
    case RightHandle:
        setCursor(Qt::SizeHorCursor);
        break;
    case TopHandle:
    case BottomHandle:
        setCursor(Qt::SizeVerCursor);
        break;
    default:
        setCursor(isSelected() ? Qt::SizeAllCursor : Qt::ArrowCursor);
        break;
    }
}

QRectF DetectionRectItem::resizedRect(const QPointF &scenePos) const
{
    QRectF r = m_pressRect;
    const QPointF localPos = mapFromScene(scenePos);

    switch (m_activeHandle) {
    case LeftHandle:        r.setLeft(localPos.x()); break;
    case RightHandle:       r.setRight(localPos.x()); break;
    case TopHandle:         r.setTop(localPos.y()); break;
    case BottomHandle:      r.setBottom(localPos.y()); break;
    case TopLeftHandle:     r.setTop(localPos.y()); r.setLeft(localPos.x()); break;
    case TopRightHandle:    r.setTop(localPos.y()); r.setRight(localPos.x()); break;
    case BottomLeftHandle:  r.setBottom(localPos.y()); r.setLeft(localPos.x()); break;
    case BottomRightHandle: r.setBottom(localPos.y()); r.setRight(localPos.x()); break;
    default: break;
    }

    return normalizedMinimumRect(r);
}

QRectF DetectionRectItem::normalizedMinimumRect(const QRectF &rect) const
{
    QRectF r = rect.normalized();

    if (r.width() < m_minSize)
        r.setWidth(m_minSize);
    if (r.height() < m_minSize)
        r.setHeight(m_minSize);

    return r.normalized();
}
