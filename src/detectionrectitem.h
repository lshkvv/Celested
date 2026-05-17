#ifndef DETECTIONRECTITEM_H
#define DETECTIONRECTITEM_H

#include <QObject>
#include <QGraphicsRectItem>

class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class DetectionRectItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    explicit DetectionRectItem(int detectionId,
                               const QRectF &rect,
                               QGraphicsItem *parent = nullptr);

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
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    enum Handle {
        NoHandle,
        LeftHandle,
        RightHandle,
        TopHandle,
        BottomHandle,
        TopLeftHandle,
        TopRightHandle,
        BottomLeftHandle,
        BottomRightHandle
    };

    QRectF handleRect(Handle handle) const;
    Handle handleAt(const QPointF &pos) const;
    void updateCursor(const QPointF &pos);
    QRectF resizedRect(const QPointF &scenePos) const;
    QRectF normalizedMinimumRect(const QRectF &rect) const;

private:
    int m_detectionId;
    bool m_resizing;
    Handle m_activeHandle;
    QRectF m_pressRect;
    qreal m_handleSize;
    qreal m_minSize;
};

#endif // DETECTIONRECTITEM_H
