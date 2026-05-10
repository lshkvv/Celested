#ifndef DETECTIONRECTITEM_H
#define DETECTIONRECTITEM_H

#include <QObject>
#include <QGraphicsRectItem>
#include <QRectF>
#include <QPointF>
#include <QVariant>

class QGraphicsSceneMouseEvent;
class QPainter;

class DetectionRectItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    explicit DetectionRectItem(int detectionId, const QRectF &rect, QGraphicsItem *parent = nullptr);

    int detectionId() const;

signals:
    void clicked(int detectionId);
    void geometryChanged(int detectionId, const QRectF &rect);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    enum DragMode {
        NoDrag,
        MoveDrag,
        ResizeBottomRight
    };

    QRectF handleRect() const;
    void updateCursor(const QPointF &localPos);

    int m_detectionId;
    QPointF m_dragStartPos;
    QRectF m_initialRect;
    DragMode m_dragMode;
    qreal m_handleSize;
    qreal m_minSize;
};

#endif // DETECTIONRECTITEM_H
