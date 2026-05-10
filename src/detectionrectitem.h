#ifndef DETECTIONRECTITEM_H
#define DETECTIONRECTITEM_H

#include <QObject>
#include <QGraphicsRectItem>
#include <QRectF>
#include <QPointF>
#include <QVariant>

class QGraphicsSceneMouseEvent;

class DetectionRectItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    explicit DetectionRectItem(int detectionId, const QRectF &rect, QGraphicsItem *parent = nullptr);

    int detectionId() const;

signals:
    void clicked(int detectionId);
    void moved(int detectionId, const QRectF &rect);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    int m_detectionId;
    QPointF m_dragStartPos;
};

#endif // DETECTIONRECTITEM_H
