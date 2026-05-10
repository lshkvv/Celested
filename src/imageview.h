#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>
#include <QRectF>
#include <QPointF>

class QGraphicsRectItem;

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    void setDrawModeEnabled(bool enabled);
    bool isDrawModeEnabled() const;

signals:
    void detectionDrawn(const QRectF &rect);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_drawModeEnabled;
    bool m_drawing;
    QPointF m_startScenePos;
    QGraphicsRectItem *m_previewRect;
};

#endif // IMAGEVIEW_H
