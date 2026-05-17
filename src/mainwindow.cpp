#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applogger.h"
#include "apptheme.h"
#include "detectionrectitem.h"
#include "imageview.h"
#include "llmclient.h"
#include "skyfielddialog.h"

#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QSqlRelation>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QItemSelectionModel>
#include <QVariant>
#include <QPushButton>
#include <QTimer>
#include <QShortcut>
#include <QKeySequence>
#include <QStatusBar>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QImageReader>
#include <QStringConverter>
#include <QMessageBox>
#include <QDialog>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QLineEdit>
#include <QMetaType>
#include <QThread>
#include <QTextCursor>
#include <QVariantMap>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , typesModel(nullptr)
    , objectsModel(nullptr)
    , detectionsModel(nullptr)
    , imageScene(new QGraphicsScene(this))
    , currentImageId(-1)
{
    ui->setupUi(this);

    qRegisterMetaType<SkyAnalysisRequest>();
    qRegisterMetaType<SkyAnalysisResult>();

    setupLogging();
    applyUiPolish();
    applySplitterDefaults();
    setupInspectorPanel();
    setupGuidePanel();
    setupAnalysisWorker();

    ui->imageView->setScene(imageScene);

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        LOG_ERROR("Database", "Database connection is not available at startup.");
        ui->objectInfoEdit->setPlainText(tr("Database is not open. Check the activity log for details."));
        appendActivityLog(tr("Database is not open. Restart the application after fixing configuration."),
                          LogLevel::Error);
        return;
    }

    typesModel = new QSqlTableModel(this, db);
    typesModel->setTable("object_types");
    typesModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    typesModel->select();
    typesModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    typesModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    typesModel->setHeaderData(2, Qt::Horizontal, tr("Description"));

    ui->typesTableView->setModel(typesModel);
    ui->typesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->typesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->typesTableView->setColumnHidden(0, true);
    ui->typesTableView->setAlternatingRowColors(true);
    ui->typesTableView->horizontalHeader()->setStretchLastSection(true);
    ui->typesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    objectsModel = new QSqlRelationalTableModel(this, db);
    objectsModel->setTable("objects");
    objectsModel->setEditStrategy(QSqlTableModel::OnRowChange);
    objectsModel->setRelation(2, QSqlRelation("object_types", "id", "name"));
    objectsModel->select();
    objectsModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    objectsModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    objectsModel->setHeaderData(2, Qt::Horizontal, tr("Type"));
    objectsModel->setHeaderData(3, Qt::Horizontal, tr("Image ID"));
    objectsModel->setHeaderData(4, Qt::Horizontal, tr("RA"));
    objectsModel->setHeaderData(5, Qt::Horizontal, tr("Dec"));
    objectsModel->setHeaderData(6, Qt::Horizontal, tr("Magnitude"));
    objectsModel->setHeaderData(7, Qt::Horizontal, tr("Constellation"));
    objectsModel->setHeaderData(8, Qt::Horizontal, tr("Messier"));
    objectsModel->setHeaderData(9, Qt::Horizontal, tr("NGC"));
    objectsModel->setHeaderData(10, Qt::Horizontal, tr("IC"));
    objectsModel->setHeaderData(11, Qt::Horizontal, tr("ID status"));

    ui->objectsTableView->setModel(objectsModel);
    ui->objectsTableView->setItemDelegate(new QSqlRelationalDelegate(ui->objectsTableView));
    ui->objectsTableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed
                                          | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    ui->objectsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->objectsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->objectsTableView->setColumnHidden(0, true);
    ui->objectsTableView->setColumnHidden(3, true);
    ui->objectsTableView->setAlternatingRowColors(true);
    ui->objectsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->objectsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->objectsTableView->horizontalHeader()->setMinimumSectionSize(72);
    ui->objectsTableView->resizeColumnsToContents();

    detectionsModel = new QSqlTableModel(this, db);
    detectionsModel->setTable("detections");
    detectionsModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    detectionsModel->select();
    detectionsModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    detectionsModel->setHeaderData(1, Qt::Horizontal, tr("Image ID"));
    detectionsModel->setHeaderData(2, Qt::Horizontal, tr("Object ID"));
    detectionsModel->setHeaderData(3, Qt::Horizontal, tr("X"));
    detectionsModel->setHeaderData(4, Qt::Horizontal, tr("Y"));
    detectionsModel->setHeaderData(5, Qt::Horizontal, tr("Width"));
    detectionsModel->setHeaderData(6, Qt::Horizontal, tr("Height"));
    detectionsModel->setHeaderData(7, Qt::Horizontal, tr("Confidence"));

    ui->detectionsTableView->setModel(detectionsModel);
    ui->detectionsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->detectionsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->detectionsTableView->setColumnHidden(1, true);
    ui->detectionsTableView->setAlternatingRowColors(true);
    ui->detectionsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->detectionsTableView->horizontalHeader()->setStretchLastSection(true);
    ui->detectionsTableView->horizontalHeader()->setMinimumSectionSize(68);
    ui->detectionsTableView->resizeColumnsToContents();

    ui->typesTableView->verticalHeader()->setVisible(false);
    ui->objectsTableView->verticalHeader()->setVisible(false);
    ui->detectionsTableView->verticalHeader()->setVisible(false);

    ui->typesTableView->setShowGrid(false);
    ui->objectsTableView->setShowGrid(false);
    ui->detectionsTableView->setShowGrid(false);

    ui->typesTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->objectsTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->detectionsTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    connect(ui->addTypeButton, &QPushButton::clicked, this, &MainWindow::addType);
    connect(ui->deleteTypeButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentType);
    connect(ui->addObjectButton, &QPushButton::clicked, this, &MainWindow::addObject);
    connect(ui->deleteObjectButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentObject);

    connect(ui->deleteDetectionButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentDetection);
    connect(ui->linkDetectionButton, &QPushButton::clicked, this, &MainWindow::linkDetectionToSelectedObject);

    connect(ui->exportJsonButton, &QPushButton::clicked, this, &MainWindow::exportAnnotationsToJson);
    connect(ui->importJsonButton, &QPushButton::clicked, this, &MainWindow::importAnnotationsFromJson);
    connect(ui->exportYoloButton, &QPushButton::clicked, this, &MainWindow::exportAnnotationsToYolo);

    connect(ui->validateButton, &QPushButton::clicked, this, &MainWindow::validateAnnotations);
    connect(ui->clampDetectionButton, &QPushButton::clicked, this, &MainWindow::clampSelectedDetectionToImage);
    connect(ui->deleteInvalidDetectionButton, &QPushButton::clicked, this, &MainWindow::deleteInvalidSelectedDetection);
    connect(ui->clampDetectionButtonInline, &QPushButton::clicked, this, &MainWindow::clampSelectedDetectionToImage);
    connect(ui->deleteInvalidDetectionButtonInline, &QPushButton::clicked, this, &MainWindow::deleteInvalidSelectedDetection);
    connect(ui->focusDetectionButton, &QPushButton::clicked, this, &MainWindow::focusSelectedDetection);

    connect(ui->openImageButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(ui->analyzeSkyButton, &QPushButton::clicked, this, &MainWindow::analyzeSky);
    connect(ui->drawDetectionButton, &QPushButton::toggled, this, &MainWindow::toggleDrawMode);
    connect(ui->sendChatButton, &QPushButton::clicked, this, &MainWindow::sendChatMessage);
    connect(ui->chatInputEdit, &QLineEdit::returnPressed, this, &MainWindow::sendChatMessage);

    connect(ui->zoomInButton, &QPushButton::clicked, ui->imageView, &ImageView::zoomIn);
    connect(ui->zoomOutButton, &QPushButton::clicked, ui->imageView, &ImageView::zoomOut);
    connect(ui->fitImageButton, &QPushButton::clicked, this, &MainWindow::fitImageIfAvailable);

    connect(ui->imageView, &ImageView::detectionDrawn, this, &MainWindow::saveDetectionFromRect);
    connect(ui->imageView, &ImageView::temporaryPanStateChanged, this, &MainWindow::handleTemporaryPanState);

    connect(ui->objectsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::handleObjectSelection);

    connect(ui->detectionsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::handleDetectionSelection);

    applyCatalogFiltersForCurrentImage();
    ui->rightTabWidget->setTabText(ui->rightTabWidget->indexOf(ui->inspectorTab), tr("Activity"));

    setupShortcuts();
    updateQuickStats();
    refreshActionStates();

    statusBar()->showMessage(
        tr("Ready — Delete: remove detection, F: fit, +/-: zoom, Esc: exit draw, Space: pan."));

    QTimer::singleShot(0, this, [this]() {
        appendActivityLog(tr("Celested ready. Open an image to start annotating."));
        LOG_INFO("App", "Main window initialized.");
    });
}

MainWindow::~MainWindow()
{
    AppLogger::instance().setUiSink(nullptr);

    if (m_analysisThread) {
        m_analysisThread->quit();
        m_analysisThread->wait(3000);
    }

    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::applyUiPolish()
{
    ui->rightPanelFrame->setMinimumWidth(520);
    ui->rightPanelFrame->setMaximumWidth(620);
    ui->imageView->setMinimumWidth(860);
    ui->imageView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->topToolbarFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->topToolbarFrame->setMaximumHeight(52);

    for (QPushButton *button : ui->topToolbarFrame->findChildren<QPushButton *>()) {
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    ui->canvasHintLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    setStyleSheet(AppTheme::applicationStyleSheet());
    ui->imageView->setStyleSheet(AppTheme::imageViewStyleSheet());
}

void MainWindow::setupGuidePanel()
{
    ui->guideSummaryEdit->setReadOnly(true);
    ui->chatHistoryEdit->setReadOnly(true);
    ui->guideSummaryEdit->setPlaceholderText(tr("Select an object to see its summary."));
    ui->chatHistoryEdit->setPlaceholderText(tr("Ask questions about the selected object."));

    m_llmClient = new LlmClient(this);
    connect(m_llmClient, &LlmClient::replyReady, this, &MainWindow::onLlmReply);
    connect(m_llmClient, &LlmClient::errorOccurred, this, &MainWindow::onLlmError);

    if (m_llmClient->isConfigured()) {
        ui->llmHintLabel->setText(tr("LLM guide is ready."));
    } else {
        ui->llmHintLabel->setText(
            tr("Set CELESTED_LLM_API_KEY (and optionally CELESTED_LLM_API_URL / CELESTED_LLM_MODEL) to chat."));
    }
}

void MainWindow::setupAnalysisWorker()
{
    m_analysisThread = new QThread(this);
    m_imageAnalyzer = new ImageAnalyzer();
    m_imageAnalyzer->moveToThread(m_analysisThread);
    connect(m_imageAnalyzer, &ImageAnalyzer::finished, this, &MainWindow::onAnalysisFinished);
    connect(m_analysisThread, &QThread::finished, m_imageAnalyzer, &QObject::deleteLater);
    m_analysisThread->start();
}

void MainWindow::setupInspectorPanel()
{
    ui->objectInfoEdit->setReadOnly(true);
    ui->validationLogEdit->setReadOnly(true);
    ui->objectInfoEdit->setPlaceholderText(tr("Select an object or detection to see details."));
    ui->validationLogEdit->setPlaceholderText(tr("Activity and validation messages appear here."));
    ui->objectInfoEdit->setPlainText(
        tr("Welcome to Celested.\n\nOpen an astrophoto, draw detections, link them to catalog objects, "
           "then validate and export."));
    ui->validationLogEdit->clear();

    ui->canvasHintLabel->setText(tr("Space pan · Wheel zoom · F fit · Esc draw"));
}

void MainWindow::setupLogging()
{
    AppLogger::instance().setUiSink([this](LogLevel level, const QString &message) {
        QMetaObject::invokeMethod(
            this,
            [this, level, message]() { appendActivityLog(message, level); },
            Qt::QueuedConnection);
    });
}

void MainWindow::appendActivityLog(const QString &message, LogLevel level)
{
    Q_UNUSED(level)

    if (!ui || !ui->validationLogEdit)
        return;

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    const QString line = QStringLiteral("[%1] %2").arg(timestamp, message);

    QPlainTextEdit *logEdit = ui->validationLogEdit;

    if (logEdit->document()->blockCount() > kMaxActivityLogLines) {
        const QString trimmed = logEdit->toPlainText();
        const QStringList lines = trimmed.split(QLatin1Char('\n'));
        logEdit->setPlainText(lines.mid(lines.size() / 4).join(QLatin1Char('\n')));
    }

    logEdit->appendPlainText(line);

    QTextCursor cursor = logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    logEdit->setTextCursor(cursor);
}

void MainWindow::applyCatalogFiltersForCurrentImage()
{
    if (!objectsModel || !detectionsModel)
        return;

    if (currentImageId < 0) {
        objectsModel->setFilter(QStringLiteral("image_id < 0"));
        detectionsModel->setFilter(QStringLiteral("image_id < 0"));
    } else {
        objectsModel->setFilter(QStringLiteral("image_id = %1").arg(currentImageId));
        detectionsModel->setFilter(QStringLiteral("image_id = %1").arg(currentImageId));
    }

    objectsModel->select();
    detectionsModel->select();
}

int MainWindow::resolveImageRecord(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    if (!db.isValid() || !db.isOpen())
        return -1;

    QSqlQuery findQuery(db);
    findQuery.prepare(QStringLiteral("SELECT id FROM images WHERE path = :path LIMIT 1"));
    findQuery.bindValue(QStringLiteral(":path"), path);

    if (findQuery.exec() && findQuery.next()) {
        const int existingId = findQuery.value(0).toInt();
        LOG_INFO("Database", QStringLiteral("Reusing existing image record id=%1 for %2")
                                .arg(existingId)
                                .arg(path));
        return existingId;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(
        QStringLiteral("INSERT INTO images (path, title, created_at) VALUES (:path, :title, datetime('now'))"));
    insertQuery.bindValue(QStringLiteral(":path"), path);
    insertQuery.bindValue(QStringLiteral(":title"), QFileInfo(path).fileName());

    if (!insertQuery.exec()) {
        logSqlFailure(QStringLiteral("insert image"), insertQuery);
        return -1;
    }

    const int newId = insertQuery.lastInsertId().toInt();
    LOG_INFO("Database", QStringLiteral("Created image record id=%1 for %2").arg(newId).arg(path));
    return newId;
}

void MainWindow::logSqlFailure(const QString &operation, const QSqlQuery &query) const
{
    LOG_ERROR("Database",
              QStringLiteral("%1 failed: %2").arg(operation, query.lastError().text()));
}

void MainWindow::applySplitterDefaults()
{
    ui->mainSplitter->setChildrenCollapsible(false);
    ui->mainSplitter->setStretchFactor(0, 10);
    ui->mainSplitter->setStretchFactor(1, 0);

    QTimer::singleShot(0, this, [this]() {
        ui->mainSplitter->setSizes({1140, 560});
    });
}

void MainWindow::fitImageIfAvailable()
{
    if (!imageScene || imageScene->sceneRect().isEmpty())
        return;

    ui->imageView->fitSceneInView();
}

void MainWindow::setupShortcuts()
{
    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    connect(deleteShortcut, &QShortcut::activated, this, [this]() {
        if (currentSelectedDetectionId() >= 0)
            deleteCurrentDetection();
    });

    auto *fitShortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    connect(fitShortcut, &QShortcut::activated, this, [this]() {
        fitImageIfAvailable();
        showStatusHint("Image fitted to view.");
    });

    auto *zoomInShortcut1 = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    connect(zoomInShortcut1, &QShortcut::activated, this, [this]() {
        ui->imageView->zoomIn();
        showStatusHint("Zoom in.");
    });

    auto *zoomInShortcut2 = new QShortcut(QKeySequence(Qt::Key_Equal), this);
    connect(zoomInShortcut2, &QShortcut::activated, this, [this]() {
        ui->imageView->zoomIn();
        showStatusHint("Zoom in.");
    });

    auto *zoomOutShortcut = new QShortcut(QKeySequence(Qt::Key_Minus), this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this]() {
        ui->imageView->zoomOut();
        showStatusHint("Zoom out.");
    });

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape, 0), this);
    connect(escapeShortcut, &QShortcut::activated, this, [this]() {
        if (ui->drawDetectionButton->isChecked()) {
            ui->drawDetectionButton->setChecked(false);
            showStatusHint("Draw mode disabled.");
        }
    });
}

void MainWindow::showStatusHint(const QString &message, int timeoutMs)
{
    statusBar()->showMessage(message, timeoutMs);
}

bool MainWindow::hasLoadedImage() const
{
    return currentImageId >= 0 && imageScene && !imageScene->sceneRect().isEmpty();
}

void MainWindow::refreshActionStates()
{
    const bool imageLoaded = hasLoadedImage();
    const bool hasObject = currentSelectedObjectId() >= 0;
    const bool hasDetection = currentSelectedDetectionId() >= 0;

    QStringList invalidIssues;
    const bool selectedDetectionInvalid = selectedDetectionIsInvalid(&invalidIssues);

    ui->analyzeSkyButton->setEnabled(imageLoaded && !m_analysisRunning);
    ui->drawDetectionButton->setEnabled(imageLoaded);
    ui->zoomInButton->setEnabled(imageLoaded);
    ui->zoomOutButton->setEnabled(imageLoaded);
    ui->fitImageButton->setEnabled(imageLoaded);

    ui->addObjectButton->setEnabled(imageLoaded);

    ui->exportJsonButton->setEnabled(imageLoaded);
    ui->importJsonButton->setEnabled(true);
    ui->exportYoloButton->setEnabled(imageLoaded);
    ui->validateButton->setEnabled(imageLoaded);

    ui->deleteTypeButton->setEnabled(ui->typesTableView->currentIndex().isValid());
    ui->deleteObjectButton->setEnabled(hasObject);

    ui->deleteDetectionButton->setEnabled(hasDetection);
    ui->linkDetectionButton->setEnabled(hasDetection && hasObject);
    ui->focusDetectionButton->setEnabled(hasDetection);

    ui->clampDetectionButton->setEnabled(hasDetection);
    ui->clampDetectionButtonInline->setEnabled(hasDetection);

    ui->deleteInvalidDetectionButton->setEnabled(hasDetection && selectedDetectionInvalid);
    ui->deleteInvalidDetectionButtonInline->setEnabled(hasDetection && selectedDetectionInvalid);

    ui->analyzeSkyButton->setToolTip(
        imageLoaded
            ? tr("Search SIMBAD for objects in this field and place them on the image.")
            : tr("Open an image first."));

    if (!imageLoaded) {
        ui->drawDetectionButton->setToolTip("Open an image first.");
        ui->exportJsonButton->setToolTip("Open an image first.");
        ui->exportYoloButton->setToolTip("Open an image first.");
        ui->validateButton->setToolTip("Open an image first.");
    } else {
        ui->drawDetectionButton->setToolTip("Draw a new detection rectangle on the image.");
        ui->exportJsonButton->setToolTip("Export annotations for the current image to JSON.");
        ui->exportYoloButton->setToolTip("Export current image annotations in YOLO format.");
        ui->validateButton->setToolTip("Validate current annotations and highlight problems.");
    }

    ui->addObjectButton->setToolTip(imageLoaded
                                        ? "Create a new object for the current image."
                                        : "Open an image first.");

    ui->deleteTypeButton->setToolTip(
        ui->typesTableView->currentIndex().isValid()
            ? "Delete selected object type."
            : "Select a type first."
        );

    ui->deleteObjectButton->setToolTip(
        hasObject
            ? "Delete selected object."
            : "Select an object first."
        );

    ui->deleteDetectionButton->setToolTip(
        hasDetection
            ? "Delete selected detection."
            : "Select a detection first."
        );

    ui->linkDetectionButton->setToolTip(
        hasDetection && hasObject
            ? "Assign the selected detection to the selected object."
            : "Select both an object and a detection."
        );

    ui->focusDetectionButton->setToolTip(
        hasDetection
            ? "Zoom the canvas to the selected detection."
            : "Select a detection first."
        );

    ui->clampDetectionButton->setToolTip(
        hasDetection
            ? "Clamp selected bbox to image bounds."
            : "Select a detection first."
        );
    ui->clampDetectionButtonInline->setToolTip(ui->clampDetectionButton->toolTip());

    const QString invalidTip = hasDetection
                                   ? (selectedDetectionInvalid
                                          ? QString("Delete selected invalid bbox.\n%1").arg(invalidIssues.join("\n"))
                                          : "Selected detection is valid.")
                                   : "Select a detection first.";

    ui->deleteInvalidDetectionButton->setToolTip(invalidTip);
    ui->deleteInvalidDetectionButtonInline->setToolTip(invalidTip);
}

void MainWindow::switchInspectorToLog(const QString &text)
{
    appendActivityLog(text);
    ui->rightTabWidget->setCurrentWidget(ui->inspectorTab);
}

void MainWindow::updateQuickStats()
{
    const int objectCount = (objectsModel && currentImageId >= 0) ? objectsModel->rowCount() : 0;
    const int detectionCount = (detectionsModel && currentImageId >= 0) ? detectionsModel->rowCount() : 0;

    ui->objectsCountValueLabel->setText(QString::number(objectCount));
    ui->detectionsCountValueLabel->setText(QString::number(detectionCount));

    if (currentImageId < 0) {
        ui->currentImageValueLabel->setText(QStringLiteral("—"));
        ui->sceneMetaLabel->setText(tr("No image loaded"));
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    if (!db.isValid() || !db.isOpen()) {
        ui->currentImageValueLabel->setText(QStringLiteral("—"));
        ui->sceneMetaLabel->setText(tr("Database unavailable"));
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT title, path FROM images WHERE id = :id LIMIT 1"));
    query.bindValue(QStringLiteral(":id"), currentImageId);

    if (query.exec() && query.next()) {
        const QString title = query.value(0).toString().trimmed();
        const QString path = query.value(1).toString();
        const QString displayTitle = title.isEmpty() ? QFileInfo(path).fileName() : title;
        ui->currentImageValueLabel->setText(displayTitle);
        ui->sceneMetaLabel->setText(
            tr("%1 · %2 objects · %3 detections").arg(displayTitle).arg(objectCount).arg(detectionCount));
    } else {
        ui->currentImageValueLabel->setText(QString::number(currentImageId));
        ui->sceneMetaLabel->setText(tr("Image #%1").arg(currentImageId));
    }
}

void MainWindow::handleTemporaryPanState(bool active)
{
    if (active)
        showStatusHint("Pan mode: hold Space and drag with mouse.");
    else
        showStatusHint("Pan mode finished.", 1000);
}

MainWindow::ValidationResult MainWindow::validateCurrentImageAnnotations() const
{
    ValidationResult result;

    if (currentImageId < 0)
        return result;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return result;

    QSqlQuery imageQuery(db);
    imageQuery.prepare("SELECT path FROM images WHERE id = :id LIMIT 1");
    imageQuery.bindValue(":id", currentImageId);

    if (!imageQuery.exec() || !imageQuery.next())
        return result;

    result.imagePath = imageQuery.value(0).toString();

    QImageReader reader(result.imagePath);
    const QSize imageSize = reader.size();
    if (!imageSize.isValid() || imageSize.width() <= 0 || imageSize.height() <= 0) {
        result.issues << "Failed to read image size.";
        return result;
    }

    const double imageWidth = static_cast<double>(imageSize.width());
    const double imageHeight = static_cast<double>(imageSize.height());

    QSqlQuery query(db);
    query.prepare(
        "SELECT d.id, d.object_id, d.x, d.y, d.width, d.height, o.type_id "
        "FROM detections d "
        "LEFT JOIN objects o ON d.object_id = o.id "
        "WHERE d.image_id = :image_id "
        "ORDER BY d.id"
        );
    query.bindValue(":image_id", currentImageId);

    if (!query.exec()) {
        result.issues << "Failed to validate detections.";
        return result;
    }

    while (query.next()) {
        ++result.checkedCount;

        const int detectionId = query.value(0).toInt();
        const bool objectIsNull = query.value(1).isNull();
        const double x = query.value(2).toDouble();
        const double y = query.value(3).toDouble();
        const double w = query.value(4).toDouble();
        const double h = query.value(5).toDouble();
        const bool typeIsNull = query.value(6).isNull();

        QStringList detectionIssues;

        if (w <= 0.0 || h <= 0.0)
            detectionIssues << "width/height must be > 0.";

        if (x < 0.0 || y < 0.0 || x + w > imageWidth || y + h > imageHeight)
            detectionIssues << "box is out of image bounds.";

        if (objectIsNull)
            detectionIssues << "not linked to any object.";
        else if (typeIsNull)
            detectionIssues << "linked object has no type_id.";

        if (!detectionIssues.isEmpty()) {
            result.issuesByDetectionId[detectionId] = detectionIssues;
            for (const QString &issue : detectionIssues)
                result.issues << QString("Detection %1: %2").arg(detectionId).arg(issue);
        }
    }

    return result;
}

QString MainWindow::buildValidationReport(const ValidationResult &result) const
{
    QString report;
    report += QString("Validation report\n\nImage: %1\nChecked detections: %2\n\n")
                  .arg(result.imagePath.isEmpty() ? "Unknown" : QFileInfo(result.imagePath).fileName())
                  .arg(result.checkedCount);

    if (result.checkedCount == 0 && result.issues.isEmpty()) {
        report += "No detections found for this image.";
        return report;
    }

    if (result.issues.isEmpty()) {
        report += "No problems found.\n\nAnnotations look valid for export.";
        return report;
    }

    report += "Problems found:\n";
    for (const QString &issue : result.issues)
        report += "• " + issue + "\n";

    return report;
}

void MainWindow::applyValidationHighlighting(const ValidationResult &result)
{
    QPen defaultPen(QColor(44, 225, 255));
    defaultPen.setWidth(2);
    QBrush defaultBrush(QColor(44, 225, 255, 30));

    QPen invalidPen(QColor(255, 72, 182));
    invalidPen.setWidth(3);
    QBrush invalidBrush(QColor(255, 72, 182, 60));

    for (auto it = detectionItemsById.begin(); it != detectionItemsById.end(); ++it) {
        auto *item = it.value();
        if (!item)
            continue;

        const int detectionId = it.key();

        if (result.issuesByDetectionId.contains(detectionId)) {
            item->setPen(invalidPen);
            item->setBrush(invalidBrush);
            item->setToolTip(
                QString("Detection %1\n%2")
                    .arg(detectionId)
                    .arg(result.issuesByDetectionId.value(detectionId).join("\n"))
                );
        } else {
            item->setPen(defaultPen);
            item->setBrush(defaultBrush);
            item->setToolTip(QString("Detection %1").arg(detectionId));
        }
    }

    highlightCurrentDetection();
}

void MainWindow::refreshValidationHighlighting()
{
    if (currentImageId < 0)
        return;

    applyValidationHighlighting(validateCurrentImageAnnotations());
}

QRectF MainWindow::clampedRectToCurrentImage(const QRectF &rect, bool *changed) const
{
    if (changed)
        *changed = false;

    if (currentImageId < 0)
        return rect;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return rect;

    QSqlQuery imageQuery(db);
    imageQuery.prepare("SELECT path FROM images WHERE id = :id LIMIT 1");
    imageQuery.bindValue(":id", currentImageId);
    if (!imageQuery.exec() || !imageQuery.next())
        return rect;

    const QString imagePath = imageQuery.value(0).toString();
    QImageReader reader(imagePath);
    const QSize imageSize = reader.size();
    if (!imageSize.isValid() || imageSize.width() <= 0 || imageSize.height() <= 0)
        return rect;

    QRectF normalized = rect.normalized();

    const double maxW = static_cast<double>(imageSize.width());
    const double maxH = static_cast<double>(imageSize.height());

    const double left = std::clamp(normalized.left(), 0.0, maxW);
    const double top = std::clamp(normalized.top(), 0.0, maxH);
    const double right = std::clamp(normalized.right(), 0.0, maxW);
    const double bottom = std::clamp(normalized.bottom(), 0.0, maxH);

    QRectF clamped(QPointF(left, top), QPointF(right, bottom));
    clamped = clamped.normalized();

    if (changed)
        *changed = (clamped != normalized);

    return clamped;
}

bool MainWindow::selectedDetectionRect(QRectF *rect, int *detectionId) const
{
    const int id = currentSelectedDetectionId();
    if (id < 0)
        return false;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return false;

    QSqlQuery query(db);
    query.prepare("SELECT x, y, width, height FROM detections WHERE id = :id LIMIT 1");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next())
        return false;

    if (rect) {
        *rect = QRectF(query.value(0).toDouble(),
                       query.value(1).toDouble(),
                       query.value(2).toDouble(),
                       query.value(3).toDouble());
    }

    if (detectionId)
        *detectionId = id;

    return true;
}

bool MainWindow::selectedDetectionIsInvalid(QStringList *issues) const
{
    const int detectionId = currentSelectedDetectionId();
    if (detectionId < 0)
        return false;

    const ValidationResult result = validateCurrentImageAnnotations();
    if (!result.issuesByDetectionId.contains(detectionId))
        return false;

    if (issues)
        *issues = result.issuesByDetectionId.value(detectionId);

    return true;
}

bool MainWindow::ensureValidForYoloExport()
{
    const ValidationResult result = validateCurrentImageAnnotations();
    const QString report = buildValidationReport(result);

    if (currentImageId < 0) {
        QMessageBox::warning(this, "YOLO export", "No image loaded.");
        showStatusHint("No image loaded.");
        return false;
    }

    applyValidationHighlighting(result);
    refreshActionStates();
    switchInspectorToLog(report);

    if (!result.issues.isEmpty()) {
        QMessageBox::critical(
            this,
            "YOLO export blocked",
            "Export cancelled because critical annotation issues were found.\n\n" + report
            );
        showStatusHint(QString("Export blocked: %1 issue(s).").arg(result.issues.size()), 4000);
        return false;
    }

    if (result.checkedCount == 0) {
        const auto answer = QMessageBox::question(
            this,
            "YOLO export",
            "No detections found for this image.\n\nExport empty YOLO labels anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (answer != QMessageBox::Yes) {
            showStatusHint("YOLO export cancelled.");
            return false;
        }
    }

    return true;
}

void MainWindow::clampSelectedDetectionToImage()
{
    QRectF rect;
    int detectionId = -1;
    if (!selectedDetectionRect(&rect, &detectionId)) {
        QMessageBox::information(this, "Clamp bbox", "Select a detection first.");
        return;
    }

    bool changed = false;
    const QRectF clamped = clampedRectToCurrentImage(rect, &changed);

    if (clamped.width() <= 0.0 || clamped.height() <= 0.0) {
        QMessageBox::warning(this, "Clamp bbox",
                             "The selected bbox cannot be clamped to a valid area inside the image.");
        return;
    }

    if (!changed) {
        showStatusHint("Selected bbox is already inside image bounds.");
        return;
    }

    updateDetectionGeometry(detectionId, clamped);
    refreshValidationHighlighting();
    refreshActionStates();
    switchInspectorToLog(QString("Detection %1 was clamped to image bounds.").arg(detectionId));
    showStatusHint("Selected bbox clamped to image.");
}

void MainWindow::deleteInvalidSelectedDetection()
{
    QStringList issues;
    if (!selectedDetectionIsInvalid(&issues)) {
        QMessageBox::information(this, "Delete invalid bbox",
                                 "The selected detection is valid or no detection is selected.");
        return;
    }

    const auto answer = QMessageBox::warning(
        this,
        "Delete invalid bbox",
        "The selected detection has validation issues:\n\n- " + issues.join("\n- ")
            + "\n\nDelete it?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
        return;

    deleteCurrentDetection();
    switchInspectorToLog("Invalid detection deleted.");
}

void MainWindow::focusSelectedDetection()
{
    const int detectionId = currentSelectedDetectionId();
    if (detectionId < 0) {
        QMessageBox::information(this, "Focus detection", "Select a detection first.");
        return;
    }

    auto it = detectionItemsById.find(detectionId);
    if (it == detectionItemsById.end() || !it.value())
        return;

    ui->imageView->fitInView(it.value()->sceneBoundingRect(), Qt::KeepAspectRatio);
    showStatusHint(QString("Focused detection %1.").arg(detectionId));
}

void MainWindow::addType()
{
    if (!typesModel)
        return;

    const int row = typesModel->rowCount();
    if (!typesModel->insertRow(row))
        return;

    const QModelIndex nameIndex = typesModel->index(row, 1);
    ui->rightTabWidget->setCurrentWidget(ui->catalogTab);
    ui->typesTableView->setCurrentIndex(nameIndex);
    ui->typesTableView->edit(nameIndex);
    refreshActionStates();
}

void MainWindow::deleteCurrentType()
{
    if (!typesModel)
        return;

    const QModelIndex index = ui->typesTableView->currentIndex();
    if (!index.isValid())
        return;

    const QString typeName = typesModel->data(typesModel->index(index.row(), 1)).toString();

    const auto answer = QMessageBox::warning(
        this,
        "Delete type",
        QString("Delete type \"%1\"?").arg(typeName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
        return;

    typesModel->removeRow(index.row());
    typesModel->select();
    LOG_INFO("Catalog", QStringLiteral("Deleted type \"%1\"").arg(typeName));
    updateQuickStats();
    refreshActionStates();
    showStatusHint(tr("Type deleted."));
}

void MainWindow::addObject()
{
    if (!objectsModel || currentImageId < 0)
        return;

    QSqlRecord record = objectsModel->record();
    record.setValue(QStringLiteral("image_id"), currentImageId);
    record.setValue(QStringLiteral("name"), QString());

    if (!objectsModel->insertRecord(-1, record)) {
        LOG_ERROR("Catalog",
                  QStringLiteral("Failed to add object: %1").arg(objectsModel->lastError().text()));
        QMessageBox::warning(this,
                             tr("Add object"),
                             tr("Could not create object:\n%1").arg(objectsModel->lastError().text()));
        return;
    }

    const int row = objectsModel->rowCount() - 1;
    const QModelIndex nameIndex = objectsModel->index(row, 1);

    ui->rightTabWidget->setCurrentWidget(ui->catalogTab);
    ui->objectsTableView->setFocus();
    ui->objectsTableView->selectRow(row);
    ui->objectsTableView->scrollTo(nameIndex);

    QTimer::singleShot(0, this, [this, nameIndex]() {
        ui->objectsTableView->setCurrentIndex(nameIndex);
        ui->objectsTableView->edit(nameIndex);
    });

    updateQuickStats();
    refreshActionStates();
    showStatusHint(tr("Enter object name in the table."));
    LOG_INFO("Catalog", QStringLiteral("Object row created for image_id=%1").arg(currentImageId));
}

void MainWindow::deleteCurrentObject()
{
    if (!objectsModel)
        return;

    const QModelIndex index = ui->objectsTableView->currentIndex();
    if (!index.isValid())
        return;

    const QString objectName = objectsModel->data(objectsModel->index(index.row(), 1)).toString();

    const auto answer = QMessageBox::warning(
        this,
        "Delete object",
        QString("Delete object \"%1\"?").arg(objectName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
        return;

    objectsModel->removeRow(index.row());
    objectsModel->select();
    updateObjectInfo();
    refreshValidationHighlighting();
    updateQuickStats();
    refreshActionStates();
    showStatusHint("Object deleted.");
}

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open image"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp)")
        );

    if (path.isEmpty())
        return;

    LOG_INFO("UI", QStringLiteral("Opening image: %1").arg(path));

    const QPixmap pix(path);
    if (pix.isNull()) {
        LOG_ERROR("UI", QStringLiteral("Failed to decode image: %1").arg(path));
        QMessageBox::warning(this, tr("Open image"), tr("Could not load the selected image file."));
        showStatusHint(tr("Failed to load image."));
        return;
    }

    const int imageId = resolveImageRecord(path);
    if (imageId < 0) {
        QMessageBox::warning(this, tr("Open image"), tr("Could not register the image in the database."));
        showStatusHint(tr("Failed to save image record."));
        return;
    }

    currentImageId = imageId;

    imageScene->clear();
    detectionItemsById.clear();

    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    fitImageIfAvailable();

    applyCatalogFiltersForCurrentImage();
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();
    refreshActionStates();

    const QString fileName = QFileInfo(path).fileName();
    ui->objectInfoEdit->setPlainText(
        tr("Image loaded: %1\n\nDraw detections on the canvas or add catalog objects in the Objects tab.")
            .arg(fileName));
    switchInspectorToLog(tr("Image opened: %1 (id %2)").arg(fileName).arg(currentImageId));
    ui->rightTabWidget->setCurrentWidget(ui->inspectorTab);

    showStatusHint(tr("Image loaded: %1").arg(fileName), 3000);
}

void MainWindow::analyzeSky()
{
    if (currentImageId < 0 || !hasLoadedImage() || m_analysisRunning || !m_imageAnalyzer)
        return;

    SkyFieldDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    QSqlQuery imageQuery(db);
    imageQuery.prepare(QStringLiteral("SELECT path FROM images WHERE id = :id LIMIT 1"));
    imageQuery.bindValue(QStringLiteral(":id"), currentImageId);
    if (!imageQuery.exec() || !imageQuery.next()) {
        QMessageBox::warning(this, tr("Analyze sky"), tr("Could not read the current image."));
        return;
    }

    const QRectF sceneRect = imageScene->sceneRect();

    SkyAnalysisRequest request;
    request.imageId = currentImageId;
    request.imagePath = imageQuery.value(0).toString();
    request.centerRaDeg = dialog.centerRaHours() * 15.0;
    request.centerDecDeg = dialog.centerDecDeg();
    request.fieldRadiusDeg = dialog.fieldRadiusDeg();
    request.imageWidth = static_cast<int>(sceneRect.width());
    request.imageHeight = static_cast<int>(sceneRect.height());
    request.replaceExisting = true;

    m_analysisRunning = true;
    ui->analyzeSkyButton->setEnabled(false);
    showStatusHint(tr("Searching SIMBAD and placing objects…"));
    switchInspectorToLog(tr("Sky analysis started (RA=%1°, Dec=%2°, radius=%3°).")
                             .arg(request.centerRaDeg, 0, 'f', 3)
                             .arg(request.centerDecDeg, 0, 'f', 3)
                             .arg(request.fieldRadiusDeg, 0, 'f', 3));

    QMetaObject::invokeMethod(
        m_imageAnalyzer,
        "analyze",
        Qt::QueuedConnection,
        Q_ARG(SkyAnalysisRequest, request));
}

void MainWindow::onAnalysisFinished(const SkyAnalysisResult &result)
{
    m_analysisRunning = false;
    refreshActionStates();

    if (!result.success) {
        QMessageBox::warning(this, tr("Analyze sky"), result.message);
        switchInspectorToLog(tr("Analysis failed: %1").arg(result.message));
        showStatusHint(tr("Analysis failed."));
        return;
    }

    typesModel->select();
    applyCatalogFiltersForCurrentImage();
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();
    refreshActionStates();

    switchInspectorToLog(result.message);
    showStatusHint(result.message, 4000);
    ui->rightTabWidget->setCurrentWidget(ui->catalogTab);

    if (objectsModel->rowCount() > 0) {
        const QModelIndex first = objectsModel->index(0, 1);
        ui->objectsTableView->setCurrentIndex(first);
    }
}

void MainWindow::sendChatMessage()
{
    const QString question = ui->chatInputEdit->text().trimmed();
    if (question.isEmpty())
        return;

    if (currentSelectedObjectId() < 0) {
        QMessageBox::information(this, tr("Guide"), tr("Select an object in the Objects tab first."));
        return;
    }

    ui->chatInputEdit->clear();
    ui->chatHistoryEdit->appendPlainText(QStringLiteral("You: %1").arg(question));
    ui->chatHistoryEdit->appendPlainText(tr("Celested: …"));
    ui->rightTabWidget->setCurrentWidget(ui->guideTab);

    m_llmClient->askAboutObject(question, buildObjectContext());
}

void MainWindow::onLlmReply(const QString &reply)
{
    QTextCursor cursor(ui->chatHistoryEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::BlockUnderCursor);
    if (cursor.selectedText().startsWith(QStringLiteral("Celested:")))
        cursor.removeSelectedText();

    ui->chatHistoryEdit->appendPlainText(QStringLiteral("Celested: %1").arg(reply));
    ui->chatHistoryEdit->appendPlainText(QString());
}

void MainWindow::onLlmError(const QString &error)
{
    ui->chatHistoryEdit->appendPlainText(QStringLiteral("Celested: %1").arg(error));
    ui->chatHistoryEdit->appendPlainText(QString());
    switchInspectorToLog(error);
}

QVariantMap MainWindow::buildObjectContext() const
{
    QVariantMap context;
    const QModelIndex current = ui->objectsTableView->currentIndex();
    if (!current.isValid() || !objectsModel)
        return context;

    const int row = current.row();
    auto cell = [&](int column) { return objectsModel->data(objectsModel->index(row, column)).toString(); };

    context.insert(QStringLiteral("name"), cell(1));
    context.insert(QStringLiteral("type"), cell(2));
    context.insert(QStringLiteral("ra"), cell(4));
    context.insert(QStringLiteral("dec"), cell(5));
    context.insert(QStringLiteral("magnitude"), cell(6));
    context.insert(QStringLiteral("constellation"), cell(7));
    context.insert(QStringLiteral("messier"), cell(8));
    context.insert(QStringLiteral("ngc"), cell(9));
    context.insert(QStringLiteral("ic"), cell(10));
    context.insert(QStringLiteral("identification_status"), cell(11));
    context.insert(QStringLiteral("guessed_type"), cell(12));
    context.insert(QStringLiteral("simbad_type"), cell(13));
    return context;
}

void MainWindow::toggleDrawMode(bool checked)
{
    ui->imageView->setDrawModeEnabled(checked);

    if (checked) {
        const int objectId = currentSelectedObjectId();
        if (objectId >= 0) {
            const QString objectName =
                objectsModel->data(objectsModel->index(ui->objectsTableView->currentIndex().row(), 1)).toString();
            ui->objectInfoEdit->setPlainText(
                QString("Draw mode enabled.\n\nCurrent target object: %1\nDraw a rectangle on the image.")
                    .arg(objectName)
                );
        } else {
            ui->objectInfoEdit->setPlainText(
                "Draw mode enabled.\n\nNo object selected.\nThe new detection will be created without object_id."
                );
        }
        ui->rightTabWidget->setCurrentWidget(ui->inspectorTab);
        showStatusHint("Draw mode enabled.");
    } else {
        updateObjectInfo();
        showStatusHint("Draw mode disabled.");
    }

    refreshActionStates();
}

void MainWindow::saveDetectionFromRect(const QRectF &rect)
{
    if (currentImageId < 0)
        return;

    bool clampedChanged = false;
    const QRectF normalizedRect = clampedRectToCurrentImage(rect.normalized(), &clampedChanged);
    if (normalizedRect.width() < 4.0 || normalizedRect.height() < 4.0) {
        LOG_WARN("Annotation", QStringLiteral("Ignored tiny detection rect %1x%2")
                                   .arg(normalizedRect.width())
                                   .arg(normalizedRect.height()));
        showStatusHint(tr("Detection is too small. Draw a larger rectangle."));
        return;
    }

    const int objectId = currentSelectedObjectId();

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT INTO detections (image_id, object_id, x, y, width, height, confidence) "
                       "VALUES (:image_id, :object_id, :x, :y, :width, :height, :confidence)"));
    query.bindValue(QStringLiteral(":image_id"), currentImageId);
    query.bindValue(QStringLiteral(":object_id"), objectId >= 0 ? QVariant(objectId) : QVariant());
    query.bindValue(QStringLiteral(":x"), normalizedRect.x());
    query.bindValue(QStringLiteral(":y"), normalizedRect.y());
    query.bindValue(QStringLiteral(":width"), normalizedRect.width());
    query.bindValue(QStringLiteral(":height"), normalizedRect.height());
    query.bindValue(QStringLiteral(":confidence"), 1.0);

    if (!query.exec()) {
        logSqlFailure(QStringLiteral("insert detection"), query);
        showStatusHint(tr("Failed to save detection."));
        return;
    }

    const int detectionId = query.lastInsertId().toInt();
    LOG_INFO("Annotation",
             QStringLiteral("Created detection id=%1 on image=%2 object=%3")
                 .arg(detectionId)
                 .arg(currentImageId)
                 .arg(objectId >= 0 ? QString::number(objectId) : QStringLiteral("none")));

    if (clampedChanged)
        appendActivityLog(tr("New detection was clamped to image bounds."));

    detectionsModel->select();
    loadDetections();
    selectDetectionById(detectionId);
    refreshValidationHighlighting();
    updateDetectionInfo();
    updateQuickStats();
    refreshActionStates();
    ui->rightTabWidget->setCurrentWidget(ui->detectionsTab);
    showStatusHint(tr("Detection created."));
}

void MainWindow::deleteCurrentDetection()
{
    const QModelIndex index = ui->detectionsTableView->currentIndex();
    if (!index.isValid())
        return;

    const QString detectionId = detectionsModel->data(detectionsModel->index(index.row(), 0)).toString();

    const auto answer = QMessageBox::warning(
        this,
        "Delete detection",
        QString("Delete detection #%1?").arg(detectionId),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
        return;

    detectionsModel->removeRow(index.row());
    detectionsModel->select();
    LOG_INFO("Annotation", QStringLiteral("Deleted detection #%1").arg(detectionId));
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();
    refreshActionStates();
    ui->objectInfoEdit->setPlainText(tr("Detection deleted."));
    showStatusHint(tr("Detection deleted."));
}

void MainWindow::linkDetectionToSelectedObject()
{
    const int detectionId = currentSelectedDetectionId();
    const int objectId = currentSelectedObjectId();

    if (detectionId < 0 || objectId < 0) {
        QMessageBox::information(this, "Assign object", "Select both a detection and an object.");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare("UPDATE detections SET object_id = :object_id WHERE id = :id");
    query.bindValue(":object_id", objectId);
    query.bindValue(":id", detectionId);

    if (!query.exec())
        return;

    detectionsModel->select();
    loadDetections();
    refreshValidationHighlighting();
    selectDetectionById(detectionId);
    updateDetectionInfo();
    refreshActionStates();
    showStatusHint("Detection linked to object.");
}

void MainWindow::exportAnnotationsToJson()
{
    if (currentImageId < 0) {
        showStatusHint("No image loaded.");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        showStatusHint("Database is not open.");
        return;
    }

    QString imagePath;
    {
        QSqlQuery imageQuery(db);
        imageQuery.prepare("SELECT path FROM images WHERE id = :id LIMIT 1");
        imageQuery.bindValue(":id", currentImageId);
        if (!imageQuery.exec() || !imageQuery.next()) {
            showStatusHint("Failed to read image info.");
            return;
        }
        imagePath = imageQuery.value(0).toString();
    }

    QJsonObject root;
    root["image_id"] = currentImageId;
    root["image_path"] = imagePath;
    root["exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray detectionsArray;

    QSqlQuery query(db);
    query.prepare("SELECT id, object_id, x, y, width, height, confidence "
                  "FROM detections WHERE image_id = :image_id ORDER BY id");
    query.bindValue(":image_id", currentImageId);

    if (!query.exec()) {
        showStatusHint("Failed to read detections.");
        return;
    }

    while (query.next()) {
        QJsonObject det;
        det["id"] = query.value(0).toInt();
        det["object_id"] = query.value(1).isNull() ? QJsonValue::Null : QJsonValue(query.value(1).toInt());
        det["x"] = query.value(2).toDouble();
        det["y"] = query.value(3).toDouble();
        det["width"] = query.value(4).toDouble();
        det["height"] = query.value(5).toDouble();
        det["confidence"] = query.value(6).toDouble();
        detectionsArray.append(det);
    }

    root["detections"] = detectionsArray;

    const QString baseName = QFileInfo(imagePath).completeBaseName();
    const QString defaultName = baseName.isEmpty() ? "annotations.json" : baseName + "_annotations.json";

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export annotations to JSON"),
        QDir::homePath() + "/" + defaultName,
        tr("JSON Files (*.json)")
        );

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        showStatusHint("Failed to open file for writing.");
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    LOG_INFO("Export", QStringLiteral("Exported JSON with %1 detections to %2").arg(detectionsArray.size()).arg(filePath));
    switchInspectorToLog(tr("Exported JSON: %1 (%2 detections)").arg(filePath).arg(detectionsArray.size()));
    refreshActionStates();
    showStatusHint(QString("Exported: %1").arg(QFileInfo(filePath).fileName()), 3000);
}

void MainWindow::importAnnotationsFromJson()
{
    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        showStatusHint("Database is not open.");
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Import annotations from JSON"),
        QDir::homePath(),
        tr("JSON Files (*.json)")
        );

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        showStatusHint("Failed to open JSON file.");
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        showStatusHint("Invalid JSON format.");
        return;
    }

    const QJsonObject root = doc.object();
    const QString imagePath = root.value("image_path").toString().trimmed();
    const QJsonArray detectionsArray = root.value("detections").toArray();

    if (imagePath.isEmpty()) {
        showStatusHint("JSON does not contain image_path.");
        return;
    }

    const QPixmap pix(imagePath);
    if (pix.isNull()) {
        showStatusHint(QString("Cannot load image: %1").arg(imagePath));
        return;
    }

    const int newImageId = resolveImageRecord(imagePath);
    if (newImageId < 0) {
        showStatusHint(tr("Failed to create image record."));
        return;
    }

    if (!db.transaction()) {
        LOG_ERROR("Import", QStringLiteral("Failed to start transaction: %1").arg(db.lastError().text()));
        showStatusHint(tr("Failed to start database transaction."));
        return;
    }

    QSqlQuery clearQuery(db);
    clearQuery.prepare(QStringLiteral("DELETE FROM detections WHERE image_id = :image_id"));
    clearQuery.bindValue(QStringLiteral(":image_id"), newImageId);
    if (!clearQuery.exec()) {
        db.rollback();
        logSqlFailure(QStringLiteral("clear detections before import"), clearQuery);
        showStatusHint(tr("Failed to prepare image for import."));
        return;
    }

    QSqlQuery detQuery(db);
    detQuery.prepare(
        "INSERT INTO detections (image_id, object_id, x, y, width, height, confidence) "
        "VALUES (:image_id, :object_id, :x, :y, :width, :height, :confidence)"
        );

    for (const QJsonValue &value : detectionsArray) {
        if (!value.isObject())
            continue;

        const QJsonObject det = value.toObject();

        detQuery.bindValue(":image_id", newImageId);
        detQuery.bindValue(":object_id", det.value("object_id").isNull() ? QVariant() : QVariant(det.value("object_id").toInt()));
        detQuery.bindValue(":x", det.value("x").toDouble());
        detQuery.bindValue(":y", det.value("y").toDouble());
        detQuery.bindValue(":width", det.value("width").toDouble());
        detQuery.bindValue(":height", det.value("height").toDouble());
        detQuery.bindValue(":confidence", det.value("confidence").toDouble(1.0));

        if (!detQuery.exec()) {
            db.rollback();
            logSqlFailure(QStringLiteral("import detection"), detQuery);
            showStatusHint(tr("Failed to import detections."));
            return;
        }
    }

    if (!db.commit()) {
        db.rollback();
        LOG_ERROR("Import", QStringLiteral("Commit failed: %1").arg(db.lastError().text()));
        showStatusHint(tr("Failed to commit imported data."));
        return;
    }

    currentImageId = newImageId;
    LOG_INFO("Import", QStringLiteral("Imported %1 detections from %2").arg(detectionsArray.size()).arg(filePath));

    imageScene->clear();
    detectionItemsById.clear();

    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    fitImageIfAvailable();

    applyCatalogFiltersForCurrentImage();
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();
    refreshActionStates();

    ui->objectInfoEdit->setPlainText(
        QString("JSON imported.\n\nImage: %1\nDetections loaded: %2")
            .arg(QFileInfo(imagePath).fileName())
            .arg(detectionsArray.size())
        );

    switchInspectorToLog(QString("JSON import completed:\n%1").arg(filePath));
    showStatusHint("JSON import completed.", 3000);
}

void MainWindow::exportAnnotationsToYolo()
{
    if (!ensureValidForYoloExport())
        return;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        showStatusHint("Database is not open.");
        return;
    }

    QString imagePath;
    {
        QSqlQuery imageQuery(db);
        imageQuery.prepare("SELECT path FROM images WHERE id = :id LIMIT 1");
        imageQuery.bindValue(":id", currentImageId);
        if (!imageQuery.exec() || !imageQuery.next()) {
            showStatusHint("Failed to read image info.");
            return;
        }
        imagePath = imageQuery.value(0).toString();
    }

    QImageReader reader(imagePath);
    const QSize imageSize = reader.size();
    if (!imageSize.isValid() || imageSize.width() <= 0 || imageSize.height() <= 0) {
        showStatusHint("Failed to read image size.");
        return;
    }

    const QString exportDir = QFileDialog::getExistingDirectory(
        this,
        tr("Select folder for YOLO export"),
        QDir::homePath()
        );

    if (exportDir.isEmpty())
        return;

    QDir dir(exportDir);

    QString baseName = QFileInfo(imagePath).completeBaseName();
    if (baseName.isEmpty())
        baseName = QString("image_%1").arg(currentImageId);

    QMap<int, int> typeIdToClassId;
    QStringList classNames;

    QSqlQuery classQuery(db);
    classQuery.prepare(
        "SELECT DISTINCT t.id, t.name "
        "FROM detections d "
        "LEFT JOIN objects o ON d.object_id = o.id "
        "LEFT JOIN object_types t ON o.type_id = t.id "
        "WHERE d.image_id = :image_id AND d.object_id IS NOT NULL AND t.id IS NOT NULL "
        "ORDER BY t.name, t.id"
        );
    classQuery.bindValue(":image_id", currentImageId);

    if (!classQuery.exec()) {
        showStatusHint("Failed to read classes.");
        return;
    }

    int nextClassId = 0;
    while (classQuery.next()) {
        const int typeId = classQuery.value(0).toInt();
        QString className = classQuery.value(1).toString().trimmed();
        if (className.isEmpty())
            className = "unknown";

        if (!typeIdToClassId.contains(typeId)) {
            typeIdToClassId[typeId] = nextClassId++;
            classNames.append(className);
        }
    }

    QFile labelsFile(dir.filePath(baseName + ".txt"));
    if (!labelsFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        showStatusHint("Failed to open YOLO label file.");
        return;
    }

    QTextStream out(&labelsFile);
    out.setEncoding(QStringConverter::Utf8);

    QSqlQuery query(db);
    query.prepare(
        "SELECT o.type_id, d.x, d.y, d.width, d.height "
        "FROM detections d "
        "LEFT JOIN objects o ON d.object_id = o.id "
        "WHERE d.image_id = :image_id "
        "ORDER BY d.id"
        );
    query.bindValue(":image_id", currentImageId);

    if (!query.exec()) {
        labelsFile.close();
        showStatusHint("Failed to read detections.");
        return;
    }

    const double imgW = static_cast<double>(imageSize.width());
    const double imgH = static_cast<double>(imageSize.height());

    while (query.next()) {
        if (query.value(0).isNull())
            continue;

        const int typeId = query.value(0).toInt();
        if (!typeIdToClassId.contains(typeId))
            continue;

        const int classId = typeIdToClassId[typeId];
        const double x = query.value(1).toDouble();
        const double y = query.value(2).toDouble();
        const double w = query.value(3).toDouble();
        const double h = query.value(4).toDouble();

        const double xc = std::clamp((x + w / 2.0) / imgW, 0.0, 1.0);
        const double yc = std::clamp((y + h / 2.0) / imgH, 0.0, 1.0);
        const double nw = std::clamp(w / imgW, 0.0, 1.0);
        const double nh = std::clamp(h / imgH, 0.0, 1.0);

        out << classId << ' '
            << QString::number(xc, 'f', 6) << ' '
            << QString::number(yc, 'f', 6) << ' '
            << QString::number(nw, 'f', 6) << ' '
            << QString::number(nh, 'f', 6) << '\n';
    }

    labelsFile.close();

    QFile classesFile(dir.filePath("classes.txt"));
    if (classesFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream classesOut(&classesFile);
        classesOut.setEncoding(QStringConverter::Utf8);
        for (const QString &name : classNames)
            classesOut << name << '\n';
        classesFile.close();
    }

    QFile yamlFile(dir.filePath("data.yaml"));
    if (yamlFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream yamlOut(&yamlFile);
        yamlOut.setEncoding(QStringConverter::Utf8);
        yamlOut << "path: .\n";
        yamlOut << "train: images/train\n";
        yamlOut << "val: images/val\n";
        yamlOut << "names:\n";
        for (int i = 0; i < classNames.size(); ++i)
            yamlOut << "  " << i << ": " << classNames.at(i) << '\n';
        yamlFile.close();
    }

    LOG_INFO("Export", QStringLiteral("YOLO export completed in %1 (%2 classes)").arg(exportDir).arg(classNames.size()));
    switchInspectorToLog(tr("YOLO export completed in %1").arg(exportDir));
    refreshActionStates();
    QMessageBox::information(this, tr("YOLO export"),
                             tr("YOLO export completed successfully.\n\nFile: %1.txt").arg(baseName));
    showStatusHint(tr("YOLO exported: %1.txt").arg(baseName), 3000);
}

void MainWindow::validateAnnotations()
{
    if (currentImageId < 0) {
        showStatusHint("No image loaded.");
        return;
    }

    const ValidationResult result = validateCurrentImageAnnotations();
    const QString report = buildValidationReport(result);

    ui->objectInfoEdit->setPlainText(report);
    switchInspectorToLog(report);
    applyValidationHighlighting(result);
    refreshActionStates();

    LOG_INFO("Validation",
             QStringLiteral("Checked %1 detections, %2 issue(s)")
                 .arg(result.checkedCount)
                 .arg(result.issues.size()));

    if (!result.issues.isEmpty()) {
        QMessageBox::warning(this, tr("Validation completed"),
                             tr("Validation found %1 issue(s).\n\nProblematic boxes are highlighted in red.")
                                 .arg(result.issues.size()));
        showStatusHint(tr("Validation found %1 issue(s).").arg(result.issues.size()), 4000);
        return;
    }

    if (result.checkedCount == 0) {
        QMessageBox::information(this, "Validation completed", "No detections found for this image.");
        showStatusHint("Validation complete: no detections.");
        return;
    }

    QMessageBox::information(this, "Validation completed",
                             "No problems found. Annotations look valid for export.");
    showStatusHint("Validation passed.", 3000);
}

void MainWindow::selectDetectionById(int detectionId)
{
    if (!detectionsModel)
        return;

    for (int row = 0; row < detectionsModel->rowCount(); ++row) {
        const int id = detectionsModel->data(detectionsModel->index(row, 0)).toInt();
        if (id == detectionId) {
            const QModelIndex index = detectionsModel->index(row, 0);
            ui->detectionsTableView->setCurrentIndex(index);
            ui->detectionsTableView->selectRow(row);
            ui->rightTabWidget->setCurrentWidget(ui->detectionsTab);
            break;
        }
    }

    refreshActionStates();
}

void MainWindow::updateDetectionGeometry(int detectionId, const QRectF &rect)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    if (!db.isValid() || !db.isOpen())
        return;

    bool changed = false;
    const QRectF clamped = clampedRectToCurrentImage(rect.normalized(), &changed);

    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE detections SET x = :x, y = :y, width = :w, height = :h WHERE id = :id"));
    query.bindValue(QStringLiteral(":x"), clamped.x());
    query.bindValue(QStringLiteral(":y"), clamped.y());
    query.bindValue(QStringLiteral(":w"), clamped.width());
    query.bindValue(QStringLiteral(":h"), clamped.height());
    query.bindValue(QStringLiteral(":id"), detectionId);

    if (!query.exec()) {
        logSqlFailure(QStringLiteral("update detection geometry"), query);
        return;
    }

    LOG_INFO("Annotation",
             QStringLiteral("Updated detection id=%1 geometry to (%2,%3 %4x%5)%6")
                 .arg(detectionId)
                 .arg(clamped.x(), 0, 'f', 1)
                 .arg(clamped.y(), 0, 'f', 1)
                 .arg(clamped.width(), 0, 'f', 1)
                 .arg(clamped.height(), 0, 'f', 1)
                 .arg(changed ? QStringLiteral(" [clamped]") : QString()));

    detectionsModel->select();
    selectDetectionById(detectionId);
    refreshValidationHighlighting();
    refreshActionStates();

    ui->objectInfoEdit->setPlainText(
        QString("Detection updated.\n\nID: %1\nx=%2  y=%3\nwidth=%4  height=%5")
            .arg(detectionId)
            .arg(clamped.x(), 0, 'f', 1)
            .arg(clamped.y(), 0, 'f', 1)
            .arg(clamped.width(), 0, 'f', 1)
            .arg(clamped.height(), 0, 'f', 1)
        );

    switchInspectorToLog(QString("Detection %1 geometry updated.").arg(detectionId));
    showStatusHint("Detection geometry updated.");
}

void MainWindow::clearDetectionItems()
{
    detectionItemsById.clear();
}

void MainWindow::loadDetections()
{
    if (currentImageId < 0)
        return;

    const auto items = imageScene->items();
    for (QGraphicsItem *item : items) {
        if (auto *rectItem = dynamic_cast<DetectionRectItem *>(item)) {
            imageScene->removeItem(rectItem);
            delete rectItem;
        }
    }

    clearDetectionItems();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare("SELECT id, x, y, width, height FROM detections WHERE image_id = :image_id");
    query.bindValue(":image_id", currentImageId);
    if (!query.exec()) {
        logSqlFailure(QStringLiteral("load detections"), query);
        return;
    }

    QPen defaultPen(QColor(44, 225, 255));
    defaultPen.setWidth(2);
    QBrush defaultBrush(QColor(44, 225, 255, 30));

    while (query.next()) {
        const int detectionId = query.value(0).toInt();
        const double x = query.value(1).toDouble();
        const double y = query.value(2).toDouble();
        const double w = query.value(3).toDouble();
        const double h = query.value(4).toDouble();

        auto *rectItem = new DetectionRectItem(detectionId, QRectF(x, y, w, h).normalized());
        rectItem->setPen(defaultPen);
        rectItem->setBrush(defaultBrush);
        rectItem->setToolTip(QString("Detection %1").arg(detectionId));

        connect(rectItem, &DetectionRectItem::clicked, this, &MainWindow::selectDetectionById);
        connect(rectItem, &DetectionRectItem::geometryChanged, this, &MainWindow::updateDetectionGeometry);

        imageScene->addItem(rectItem);
        detectionItemsById[detectionId] = rectItem;
    }

    LOG_DEBUG("Annotation",
              QStringLiteral("Loaded %1 detection overlays for image %2")
                  .arg(detectionItemsById.size())
                  .arg(currentImageId));
}

void MainWindow::highlightDetectionForObject(int objectId)
{
    if (currentImageId < 0 || objectId < 0)
        return;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare("SELECT id FROM detections WHERE image_id = :image_id AND object_id = :object_id LIMIT 1");
    query.bindValue(":image_id", currentImageId);
    query.bindValue(":object_id", objectId);

    if (!query.exec() || !query.next())
        return;

    selectDetectionById(query.value(0).toInt());
    highlightCurrentDetection();
}

void MainWindow::highlightCurrentDetection()
{
    const int detectionId = currentSelectedDetectionId();

    QPen defaultPen(QColor(44, 225, 255));
    defaultPen.setWidth(2);
    QBrush defaultBrush(QColor(44, 225, 255, 30));

    QPen invalidPen(QColor(255, 72, 182));
    invalidPen.setWidth(3);
    QBrush invalidBrush(QColor(255, 72, 182, 60));

    QPen activePen(QColor(155, 120, 255));
    activePen.setWidth(3);
    QBrush activeBrush(QColor(155, 120, 255, 45));

    QPen activeInvalidPen(QColor(255, 120, 0));
    activeInvalidPen.setWidth(4);
    QBrush activeInvalidBrush(QColor(255, 120, 0, 70));

    const ValidationResult validation = validateCurrentImageAnnotations();

    for (auto it = detectionItemsById.begin(); it != detectionItemsById.end(); ++it) {
        const int id = it.key();
        auto *item = it.value();
        if (!item)
            continue;

        const bool invalid = validation.issuesByDetectionId.contains(id);
        const bool selected = (id == detectionId);

        if (selected && invalid) {
            item->setPen(activeInvalidPen);
            item->setBrush(activeInvalidBrush);
        } else if (selected) {
            item->setPen(activePen);
            item->setBrush(activeBrush);
        } else if (invalid) {
            item->setPen(invalidPen);
            item->setBrush(invalidBrush);
        } else {
            item->setPen(defaultPen);
            item->setBrush(defaultBrush);
        }
    }
}

void MainWindow::updateObjectInfo()
{
    const QModelIndex current = ui->objectsTableView->currentIndex();
    if (!current.isValid()) {
        ui->objectInfoEdit->setPlainText(tr("No object selected."));
        updateGuideSummary();
        return;
    }

    const int row = current.row();
    auto cell = [&](int column) { return objectsModel->data(objectsModel->index(row, column)).toString(); };

    const QString name = cell(1);
    const QString type = cell(2);
    const QString ra = cell(4);
    const QString dec = cell(5);
    const QString mag = cell(6);
    const QString constellation = cell(7);
    const QString messier = cell(8);
    const QString ngc = cell(9);
    const QString ic = cell(10);
    const QString idStatus = cell(11);
    const QString guessedType = cell(12);
    const QString simbadType = cell(13);

    ui->objectInfoEdit->setPlainText(
        tr("Object summary\n\n"
           "Name: %1\nType: %2\nRA: %3 h  Dec: %4°\n"
           "Magnitude: %5\nConstellation: %6\n"
           "Messier: %7  NGC: %8  IC: %9\n"
           "Identification: %10\nGuessed class: %11\nSIMBAD type: %12")
            .arg(name,
                 type,
                 ra,
                 dec,
                 mag.isEmpty() ? QStringLiteral("—") : mag,
                 constellation.isEmpty() ? QStringLiteral("—") : constellation,
                 messier.isEmpty() ? QStringLiteral("—") : messier,
                 ngc.isEmpty() ? QStringLiteral("—") : ngc,
                 ic.isEmpty() ? QStringLiteral("—") : ic,
                 idStatus.isEmpty() ? QStringLiteral("—") : idStatus,
                 guessedType.isEmpty() ? QStringLiteral("—") : guessedType,
                 simbadType.isEmpty() ? QStringLiteral("—") : simbadType));

    updateGuideSummary();
}

void MainWindow::updateGuideSummary()
{
    const QModelIndex current = ui->objectsTableView->currentIndex();
    if (!current.isValid() || !objectsModel) {
        ui->guideSummaryEdit->setPlainText(
            tr("Select an object in the Objects list to see its catalog data and ask the guide questions."));
        return;
    }

    const int row = current.row();
    auto cell = [&](int column) { return objectsModel->data(objectsModel->index(row, column)).toString(); };

    const QString idStatus = cell(11);
    QString statusLine;
    if (idStatus == QStringLiteral("matched")) {
        statusLine = tr("Catalog match found.");
    } else if (idStatus == QStringLiteral("guessed")) {
        statusLine = tr("No exact catalog ID — likely class: %1").arg(cell(12));
    } else if (idStatus == QStringLiteral("not_found")) {
        statusLine = tr("Not identified in major catalogs.");
    } else {
        statusLine = tr("Identification status: %1").arg(idStatus.isEmpty() ? tr("unknown") : idStatus);
    }

    ui->guideSummaryEdit->setPlainText(
        tr("%1\n\n%2\nType: %3\nRA %4 h · Dec %5° · Mag %6")
            .arg(cell(1),
                 statusLine,
                 cell(2).isEmpty() ? cell(12) : cell(2),
                 cell(4),
                 cell(5),
                 cell(6).isEmpty() ? QStringLiteral("—") : cell(6)));
}

void MainWindow::updateDetectionInfo()
{
    const QModelIndex current = ui->detectionsTableView->currentIndex();
    if (!current.isValid())
        return;

    const int row = current.row();
    const QString detectionId = detectionsModel->data(detectionsModel->index(row, 0)).toString();
    const QString objectId = detectionsModel->data(detectionsModel->index(row, 2)).toString();
    const QString x = detectionsModel->data(detectionsModel->index(row, 3)).toString();
    const QString y = detectionsModel->data(detectionsModel->index(row, 4)).toString();
    const QString w = detectionsModel->data(detectionsModel->index(row, 5)).toString();
    const QString h = detectionsModel->data(detectionsModel->index(row, 6)).toString();
    const QString confidence = detectionsModel->data(detectionsModel->index(row, 7)).toString();

    ui->objectInfoEdit->setPlainText(
        QString("Detection summary\n\n"
                "Detection ID: %1\nObject ID: %2\n"
                "X: %3  Y: %4\nWidth: %5  Height: %6\n"
                "Confidence: %7\n\n"
                "You can move and resize this rectangle directly on the image.")
            .arg(detectionId, objectId, x, y, w, h, confidence)
        );
}

void MainWindow::handleObjectSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    const int objectId = currentSelectedObjectId();
    if (objectId >= 0)
        highlightDetectionForObject(objectId);

    ui->rightTabWidget->setCurrentWidget(ui->guideTab);
    updateObjectInfo();
    refreshActionStates();
}

void MainWindow::handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    ui->rightTabWidget->setCurrentWidget(ui->detectionsTab);
    highlightCurrentDetection();
    updateDetectionInfo();
    refreshActionStates();
}

int MainWindow::currentSelectedObjectId() const
{
    const QModelIndex current = ui->objectsTableView->currentIndex();
    if (!current.isValid())
        return -1;

    return objectsModel->data(objectsModel->index(current.row(), 0)).toInt();
}

int MainWindow::currentSelectedDetectionId() const
{
    const QModelIndex current = ui->detectionsTableView->currentIndex();
    if (!current.isValid())
        return -1;

    return detectionsModel->data(detectionsModel->index(current.row(), 0)).toInt();
}
