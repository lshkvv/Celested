#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QRectF>
#include <QModelIndex>
#include <QStringList>

class QSqlTableModel;
class QSqlRelationalTableModel;
class QGraphicsScene;
class DetectionRectItem;
class QResizeEvent;

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

protected:
    void resizeEvent(QResizeEvent *event) override;

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
    void validateAnnotations();

    void clampSelectedDetectionToImage();
    void deleteInvalidSelectedDetection();
    void focusSelectedDetection();

    void handleObjectSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleTemporaryPanState(bool active);

    void selectDetectionById(int detectionId);
    void updateDetectionGeometry(int detectionId, const QRectF &rect);

private:
    struct ValidationResult {
        QString imagePath;
        int checkedCount = 0;
        QStringList issues;
        QMap<int, QStringList> issuesByDetectionId;
    };

    ValidationResult validateCurrentImageAnnotations() const;
    QString buildValidationReport(const ValidationResult &result) const;
    bool ensureValidForYoloExport();
    void applyValidationHighlighting(const ValidationResult &result);
    void refreshValidationHighlighting();

    QRectF clampedRectToCurrentImage(const QRectF &rect, bool *changed = nullptr) const;
    bool selectedDetectionRect(QRectF *rect, int *detectionId = nullptr) const;
    bool selectedDetectionIsInvalid(QStringList *issues = nullptr) const;

    void clearDetectionItems();
    void createTestDetection();
    void loadDetections();
    void highlightDetectionForObject(int objectId);
    void highlightCurrentDetection();
    void updateObjectInfo();
    void updateDetectionInfo();
    void updateQuickStats();
    void switchInspectorToLog(const QString &text);

    int currentSelectedObjectId() const;
    int currentSelectedDetectionId() const;

    void setupShortcuts();
    void showStatusHint(const QString &message, int timeoutMs = 2000);
    void applyUiPolish();
    void applySplitterDefaults();
    void fitImageIfAvailable();

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
