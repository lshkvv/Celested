#include "detectionrectitem.h"

#include <QGraphicsSceneMouseEvent>

DetectionRectItem::DetectionRectItem(int detectionId, const QRectF &rect, QGraphicsItem *parent)
    : QObject()
    , QGraphicsRectItem(rect, parent)
    , m_detectionId(detectionId)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
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
    m_dragStartPos = pos();
    emit clicked(m_detectionId);
    QGraphicsRectItem::mousePressEvent(event);
}

void DetectionRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsRectItem::mouseReleaseEvent(event);

    if (pos() != m_dragStartPos) {
        const QRectF sceneRect = mapRectToScene(rect());
        emit moved(m_detectionId, sceneRect);
    }

    emit clicked(m_detectionId);
}
