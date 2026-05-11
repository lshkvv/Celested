#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QRectF>
#include <QModelIndex>

class QSqlTableModel;
class QSqlRelationalTableModel;
class QGraphicsScene;
class DetectionRectItem;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addType();
    void deleteCurrentType();

    void addObject();
    void deleteCurrentObject();

    void openImage();
    void toggleDrawMode(bool checked);

    void saveDetectionFromRect(const QRectF &rect);
    void deleteCurrentDetection();
    void linkDetectionToSelectedObject();

    void exportAnnotationsToJson();
    void importAnnotationsFromJson();
    void exportAnnotationsToYolo();

    void handleObjectSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleTemporaryPanState(bool active);

    void selectDetectionById(int detectionId);
    void updateDetectionGeometry(int detectionId, const QRectF &rect);

private:
    void clearDetectionItems();
    void createTestDetection();
    void loadDetections();
    void highlightDetectionForObject(int objectId);
    void highlightCurrentDetection();
    void updateObjectInfo();
    void updateDetectionInfo();

    int currentSelectedObjectId() const;
    int currentSelectedDetectionId() const;

    void setupShortcuts();
    void showStatusHint(const QString &message, int timeoutMs = 2000);

private:
    Ui::MainWindow *ui;
    QSqlTableModel *typesModel;
    QSqlRelationalTableModel *objectsModel;
    QSqlTableModel *detectionsModel;
    QGraphicsScene *imageScene;
    int currentImageId;
    QMap<int, DetectionRectItem*> detectionItemsById;
};

#endif // MAINWINDOW_H
