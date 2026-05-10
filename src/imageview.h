#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>
#include <QRectF>
#include <QPointF>

class QGraphicsRectItem;
class QWheelEvent;
class QResizeEvent;

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    void setDrawModeEnabled(bool enabled);
    bool isDrawModeEnabled() const;

    void zoomIn();
    void zoomOut();
    void fitSceneInView();

signals:
    void detectionDrawn(const QRectF &rect);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyZoom(qreal factor);

    bool m_drawModeEnabled;
    bool m_drawing;
    bool m_fitMode;
    QPointF m_startScenePos;
    QGraphicsRectItem *m_previewRect;
};

#endif // IMAGEVIEW_H
