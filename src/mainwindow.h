#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QRectF>

#include "applogger.h"
#include "imageanalyzer.h"

class QSqlQuery;
class QThread;
class LlmClient;
class QSqlTableModel;
class QSqlRelationalTableModel;
class QGraphicsScene;
class QModelIndex;
class QResizeEvent;
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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void addType();
    void deleteCurrentType();

    void addObject();
    void deleteCurrentObject();

    void openImage();
    void analyzeSky();
    void sendChatMessage();
    void onAnalysisFinished(const SkyAnalysisResult &result);
    void onLlmReply(const QString &reply);
    void onLlmError(const QString &error);

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

    void selectDetectionById(int detectionId);
    void updateDetectionGeometry(int detectionId, const QRectF &rect);

    void handleObjectSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous);
    void handleTemporaryPanState(bool active);

private:
    struct ValidationResult {
        QString imagePath;
        int checkedCount = 0;
        QStringList issues;
        QMap<int, QStringList> issuesByDetectionId;
    };

    void applyUiPolish();
    void applySplitterDefaults();
    void setupInspectorPanel();
    void setupGuidePanel();
    void setupAnalysisWorker();
    void setupLogging();
    void fitImageIfAvailable();
    void setupShortcuts();
    void showStatusHint(const QString &message, int timeoutMs = 2500);
    void appendActivityLog(const QString &message, LogLevel level = LogLevel::Info);

    int resolveImageRecord(const QString &path);
    void applyCatalogFiltersForCurrentImage();
    void logSqlFailure(const QString &operation, const QSqlQuery &query) const;

    bool hasLoadedImage() const;
    void refreshActionStates();
    void switchInspectorToLog(const QString &text);
    void updateQuickStats();

    ValidationResult validateCurrentImageAnnotations() const;
    QString buildValidationReport(const ValidationResult &result) const;
    void applyValidationHighlighting(const ValidationResult &result);
    void refreshValidationHighlighting();

    bool ensureValidForYoloExport();

    QRectF clampedRectToCurrentImage(const QRectF &rect, bool *changed = nullptr) const;
    bool selectedDetectionRect(QRectF *rect, int *detectionId = nullptr) const;
    bool selectedDetectionIsInvalid(QStringList *issues = nullptr) const;

    void clearDetectionItems();
    void loadDetections();
    void highlightDetectionForObject(int objectId);
    void highlightCurrentDetection();

    void updateObjectInfo();
    void updateGuideSummary();
    void updateDetectionInfo();
    QVariantMap buildObjectContext() const;

    int currentSelectedObjectId() const;
    int currentSelectedDetectionId() const;

private:
    Ui::MainWindow *ui = nullptr;

    QSqlTableModel *typesModel = nullptr;
    QSqlRelationalTableModel *objectsModel = nullptr;
    QSqlTableModel *detectionsModel = nullptr;

    QGraphicsScene *imageScene = nullptr;
    int currentImageId = -1;

    QMap<int, DetectionRectItem*> detectionItemsById;

    QThread *m_analysisThread = nullptr;
    ImageAnalyzer *m_imageAnalyzer = nullptr;
    LlmClient *m_llmClient = nullptr;
    bool m_analysisRunning = false;

    static constexpr int kMaxActivityLogLines = 500;
};

#endif // MAINWINDOW_H
