#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>
#include <QRectF>

class QRubberBand;

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    void setDrawModeEnabled(bool enabled);
    bool drawModeEnabled() const;

public slots:
    void zoomIn();
    void zoomOut();
    void fitSceneInView();

signals:
    void detectionDrawn(const QRectF &rect);
    void temporaryPanStateChanged(bool active);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    QRectF currentSceneRectFromRubberBand() const;
    void setTemporaryPanEnabled(bool enabled);

private:
    bool m_drawModeEnabled;
    bool m_drawing;
    bool m_temporaryPanActive;
    QPoint m_origin;
    QRubberBand *m_rubberBand;
};

#endif // IMAGEVIEW_H
