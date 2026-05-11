#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "detectionrectitem.h"
#include "imageview.h"

#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QSqlRelation>
#include <QSqlQuery>
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
#include <QResizeEvent>
#include <QSizePolicy>
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

    applyUiPolish();
    applySplitterDefaults();

    ui->objectInfoEdit->setReadOnly(true);
    ui->validationLogEdit->setReadOnly(true);
    ui->objectInfoEdit->setPlainText("Open an image to begin.");
    ui->validationLogEdit->setPlainText("Validation output will appear here.");
    ui->imageView->setScene(imageScene);

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        ui->objectInfoEdit->setPlainText("Database is not open.");
        ui->validationLogEdit->setPlainText("Database is not open.");
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
    objectsModel->setEditStrategy(QSqlTableModel::OnFieldChange);
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

    ui->objectsTableView->setModel(objectsModel);
    ui->objectsTableView->setItemDelegate(new QSqlRelationalDelegate(ui->objectsTableView));
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
    connect(ui->drawDetectionButton, &QPushButton::toggled, this, &MainWindow::toggleDrawMode);

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

    setupShortcuts();
    updateQuickStats();

    statusBar()->showMessage(
        "Ready. Shortcuts: Delete — remove detection, F — fit image, +/- — zoom, Esc — exit draw mode, Space — pan."
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (currentImageId >= 0 && !ui->drawDetectionButton->isChecked()) {
        static QTimer *resizeFitTimer = nullptr;
        if (!resizeFitTimer) {
            resizeFitTimer = new QTimer(this);
            resizeFitTimer->setSingleShot(true);
            connect(resizeFitTimer, &QTimer::timeout, this, [this]() {
                fitImageIfAvailable();
            });
        }
        resizeFitTimer->start(70);
    }
}

void MainWindow::applyUiPolish()
{
    ui->rightPanelFrame->setMinimumWidth(520);
    ui->rightPanelFrame->setMaximumWidth(620);
    ui->imageView->setMinimumWidth(860);
    ui->imageView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #070b14;
            color: #e7eeff;
            font-size: 13px;
        }

        QFrame#topToolbarFrame,
        QFrame#imagePanelFrame,
        QFrame#rightPanelFrame {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #0c1223,
                stop:0.55 #101938,
                stop:1 #16153a
            );
            border: 1px solid #2c3768;
            border-radius: 12px;
        }

        QFrame#imagePanelHeaderFrame,
        QFrame#statsFrame {
            background: transparent;
            border: none;
        }

        QLabel#brandLabel {
            color: #f2f5ff;
            font-size: 15px;
            font-weight: 700;
        }

        QLabel#imageToolsLabel,
        QLabel#ioToolsLabel,
        QLabel#validationToolsLabel,
        QLabel#imagePanelLabel {
            color: #8ca8ff;
            font-weight: 600;
        }

        QLabel#canvasHintLabel,
        QLabel#sceneMetaLabel {
            color: #7f91c7;
        }

        QLabel#statsImageTitleLabel,
        QLabel#statsObjectsTitleLabel,
        QLabel#statsDetectionsTitleLabel {
            color: #87a0ff;
            font-size: 11px;
            font-weight: 600;
        }

        QLabel#currentImageValueLabel,
        QLabel#objectsCountValueLabel,
        QLabel#detectionsCountValueLabel {
            color: #f0f4ff;
            font-size: 18px;
            font-weight: 700;
            padding: 2px 6px 6px 0;
        }

        QPushButton {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #151d36,
                stop:1 #1c2648
            );
            color: #eaf0ff;
            border: 1px solid #344070;
            border-radius: 8px;
            padding: 4px 10px;
            min-height: 24px;
        }

        QPushButton:hover {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #1a2853,
                stop:1 #24356a
            );
            border: 1px solid #5aa7ff;
        }

        QPushButton:pressed {
            background: #28396d;
            border: 1px solid #83ccff;
        }

        QPushButton:checked {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #352266,
                stop:1 #4f2f96
            );
            border: 1px solid #a46bff;
            color: #faf3ff;
            font-weight: 600;
        }

        QPushButton#openImageButton,
        QPushButton#validateButton,
        QPushButton#exportYoloButton,
        QPushButton#focusDetectionButton {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #3b2477,
                stop:1 #6138c0
            );
            border: 1px solid #9a7eff;
            color: #f9f4ff;
            font-weight: 600;
        }

        QPushButton#deleteDetectionButton,
        QPushButton#deleteInvalidDetectionButton,
        QPushButton#deleteInvalidDetectionButtonInline,
        QPushButton#deleteTypeButton,
        QPushButton#deleteObjectButton {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #35142d,
                stop:1 #511a3d
            );
            border: 1px solid #8f416f;
            color: #ffdcef;
        }

        QGroupBox {
            border: 1px solid #2e3866;
            border-radius: 11px;
            margin-top: 8px;
            padding-top: 6px;
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #0e152a,
                stop:1 #141d38
            );
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 6px;
            color: #bdd0ff;
            background: transparent;
        }

        QTabWidget::pane {
            border: 1px solid #2d3866;
            border-radius: 10px;
            top: -1px;
            background: #0b1020;
        }

        QTabBar::tab {
            background: #131c37;
            color: #9fb4eb;
            border: 1px solid #2e3968;
            padding: 8px 14px;
            min-width: 92px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 4px;
        }

        QTabBar::tab:selected {
            background: #1d2a54;
            color: #f4f7ff;
            border-color: #5977d9;
            font-weight: 600;
        }

        QTableView {
            background: #09101d;
            alternate-background-color: #0d1630;
            color: #e8efff;
            border: 1px solid #2b3562;
            border-radius: 9px;
            selection-background-color: #3f2d7b;
            selection-color: #ffffff;
            outline: 0;
        }

        QTableView::item {
            padding: 6px;
            background: transparent;
        }

        QHeaderView::section {
            background: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #172346,
                stop:1 #121c39
            );
            color: #acd0ff;
            padding: 6px 8px;
            border: none;
            border-right: 1px solid #2d3766;
            border-bottom: 1px solid #2d3766;
            font-weight: 600;
        }

        QPlainTextEdit {
            background: #09101d;
            color: #dfe7ff;
            border: 1px solid #2b3562;
            border-radius: 9px;
            padding: 8px;
            selection-background-color: #4d34a0;
            selection-color: #ffffff;
        }

        QSplitter::handle {
            background: #1a2340;
        }

        QSplitter::handle:horizontal {
            width: 6px;
        }

        QStatusBar {
            background: #0a1020;
            color: #b8c7f3;
            border-top: 1px solid #232d55;
        }
    )");

    ui->imageView->setStyleSheet(R"(
        QGraphicsView {
            background: qradialgradient(
                cx:0.5, cy:0.45, radius:1.08,
                fx:0.5, fy:0.45,
                stop:0 #16203e,
                stop:0.48 #0d1327,
                stop:1 #05070c
            );
            border: 1px solid #2b3562;
            border-radius: 10px;
        }
    )");
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
        if (currentSelectedDetectionId() > 0)
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

void MainWindow::switchInspectorToLog(const QString &text)
{
    ui->validationLogEdit->setPlainText(text);
    ui->rightTabWidget->setCurrentWidget(ui->inspectorTab);
}

void MainWindow::updateQuickStats()
{
    ui->objectsCountValueLabel->setText(QString::number(objectsModel ? objectsModel->rowCount() : 0));
    ui->detectionsCountValueLabel->setText(QString::number(detectionsModel ? detectionsModel->rowCount() : 0));

    if (currentImageId < 0) {
        ui->currentImageValueLabel->setText("—");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        ui->currentImageValueLabel->setText("—");
        return;
    }

    QSqlQuery query(db);
    query.prepare("SELECT title FROM images WHERE id = :id LIMIT 1");
    query.bindValue(":id", currentImageId);

    if (query.exec() && query.next()) {
        const QString title = query.value(0).toString().trimmed();
        ui->currentImageValueLabel->setText(title.isEmpty() ? QString::number(currentImageId) : title);
    } else {
        ui->currentImageValueLabel->setText(QString::number(currentImageId));
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
}

void MainWindow::deleteCurrentType()
{
    if (!typesModel)
        return;

    const QModelIndex index = ui->typesTableView->currentIndex();
    if (!index.isValid())
        return;

    typesModel->removeRow(index.row());
    typesModel->select();
    updateQuickStats();
    showStatusHint("Type deleted.");
}

void MainWindow::addObject()
{
    if (!objectsModel || currentImageId < 0)
        return;

    const int row = objectsModel->rowCount();
    if (!objectsModel->insertRow(row))
        return;

    objectsModel->setData(objectsModel->index(row, 3), currentImageId);

    const QModelIndex nameIndex = objectsModel->index(row, 1);
    ui->rightTabWidget->setCurrentWidget(ui->catalogTab);
    ui->objectsTableView->setCurrentIndex(nameIndex);
    ui->objectsTableView->edit(nameIndex);
    updateQuickStats();
    showStatusHint("Object added.");
}

void MainWindow::deleteCurrentObject()
{
    if (!objectsModel)
        return;

    const QModelIndex index = ui->objectsTableView->currentIndex();
    if (!index.isValid())
        return;

    objectsModel->removeRow(index.row());
    objectsModel->select();
    updateObjectInfo();
    refreshValidationHighlighting();
    updateQuickStats();
    showStatusHint("Object deleted.");
}

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open image"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)")
        );

    if (path.isEmpty())
        return;

    const QPixmap pix(path);
    if (pix.isNull()) {
        showStatusHint("Failed to load image.");
        return;
    }

    imageScene->clear();
    detectionItemsById.clear();

    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    fitImageIfAvailable();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        showStatusHint("Database is not open.");
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO images (path, title, created_at) VALUES (:path, :title, datetime('now'))");
    query.bindValue(":path", path);
    query.bindValue(":title", QFileInfo(path).fileName());

    if (!query.exec()) {
        currentImageId = -1;
        showStatusHint("Failed to save image record.");
        return;
    }

    currentImageId = query.lastInsertId().toInt();

    objectsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    objectsModel->select();

    detectionsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    detectionsModel->select();

    createTestDetection();
    detectionsModel->select();
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();

    const QString msg = QString("Image loaded:\n%1\n\nUse mouse wheel or zoom buttons to inspect details.")
                            .arg(QFileInfo(path).fileName());

    ui->objectInfoEdit->setPlainText(msg);
    switchInspectorToLog(QString("Image opened: %1").arg(QFileInfo(path).fileName()));
    ui->rightTabWidget->setCurrentWidget(ui->inspectorTab);

    showStatusHint(QString("Image loaded: %1").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::toggleDrawMode(bool checked)
{
    ui->imageView->setDrawModeEnabled(checked);

    if (checked) {
        const int objectId = currentSelectedObjectId();
        if (objectId > 0) {
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
}

void MainWindow::saveDetectionFromRect(const QRectF &rect)
{
    if (currentImageId < 0)
        return;

    const int objectId = currentSelectedObjectId();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO detections (image_id, object_id, x, y, width, height, confidence) "
        "VALUES (:image_id, :object_id, :x, :y, :width, :height, :confidence)"
        );
    query.bindValue(":image_id", currentImageId);
    query.bindValue(":object_id", objectId > 0 ? QVariant(objectId) : QVariant());
    query.bindValue(":x", rect.x());
    query.bindValue(":y", rect.y());
    query.bindValue(":width", rect.width());
    query.bindValue(":height", rect.height());
    query.bindValue(":confidence", 1.0);

    if (!query.exec())
        return;

    detectionsModel->select();
    loadDetections();
    refreshValidationHighlighting();
    updateDetectionInfo();
    updateQuickStats();
    ui->rightTabWidget->setCurrentWidget(ui->detectionsTab);
    showStatusHint("Detection created.");
}

void MainWindow::deleteCurrentDetection()
{
    const QModelIndex index = ui->detectionsTableView->currentIndex();
    if (!index.isValid())
        return;

    detectionsModel->removeRow(index.row());
    detectionsModel->select();
    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();
    ui->objectInfoEdit->setPlainText("Detection deleted.");
    showStatusHint("Detection deleted.");
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

    switchInspectorToLog(QString("Exported JSON:\n%1").arg(filePath));
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

    if (!db.transaction()) {
        showStatusHint("Failed to start database transaction.");
        return;
    }

    QSqlQuery imageQuery(db);
    imageQuery.prepare("INSERT INTO images (path, title, created_at) VALUES (:path, :title, datetime('now'))");
    imageQuery.bindValue(":path", imagePath);
    imageQuery.bindValue(":title", QFileInfo(imagePath).fileName());

    if (!imageQuery.exec()) {
        db.rollback();
        showStatusHint("Failed to create image record.");
        return;
    }

    const int newImageId = imageQuery.lastInsertId().toInt();

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
            showStatusHint("Failed to import detections.");
            return;
        }
    }

    if (!db.commit()) {
        db.rollback();
        showStatusHint("Failed to commit imported data.");
        return;
    }

    currentImageId = newImageId;

    imageScene->clear();
    detectionItemsById.clear();

    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    fitImageIfAvailable();

    objectsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    objectsModel->select();

    detectionsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    detectionsModel->select();

    loadDetections();
    refreshValidationHighlighting();
    updateQuickStats();

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

    switchInspectorToLog(QString("YOLO export completed.\nDirectory: %1").arg(exportDir));
    QMessageBox::information(this, "YOLO export",
                             QString("YOLO export completed successfully.\n\nFile: %1.txt").arg(baseName));
    showStatusHint(QString("YOLO exported: %1.txt").arg(baseName), 3000);
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

    if (!result.issues.isEmpty()) {
        QMessageBox::warning(this, "Validation completed",
                             QString("Validation found %1 issue(s).\n\nProblematic boxes are highlighted in red.")
                                 .arg(result.issues.size()));
        showStatusHint(QString("Validation found %1 issue(s).").arg(result.issues.size()), 4000);
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
}

void MainWindow::updateDetectionGeometry(int detectionId, const QRectF &rect)
{
    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare("UPDATE detections SET x = :x, y = :y, width = :w, height = :h WHERE id = :id");
    query.bindValue(":x", rect.x());
    query.bindValue(":y", rect.y());
    query.bindValue(":w", rect.width());
    query.bindValue(":h", rect.height());
    query.bindValue(":id", detectionId);

    if (!query.exec())
        return;

    detectionsModel->select();
    selectDetectionById(detectionId);
    refreshValidationHighlighting();

    ui->objectInfoEdit->setPlainText(
        QString("Detection updated.\n\nID: %1\nx=%2  y=%3\nwidth=%4  height=%5")
            .arg(detectionId)
            .arg(rect.x(), 0, 'f', 1)
            .arg(rect.y(), 0, 'f', 1)
            .arg(rect.width(), 0, 'f', 1)
            .arg(rect.height(), 0, 'f', 1)
        );

    switchInspectorToLog(QString("Detection %1 geometry updated.").arg(detectionId));
    showStatusHint("Detection geometry updated.");
}

void MainWindow::clearDetectionItems()
{
    detectionItemsById.clear();
}

void MainWindow::createTestDetection()
{
    if (currentImageId < 0)
        return;

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT COUNT(*) FROM detections WHERE image_id = :image_id");
    checkQuery.bindValue(":image_id", currentImageId);
    if (!checkQuery.exec() || !checkQuery.next())
        return;
    if (checkQuery.value(0).toInt() > 0)
        return;

    QSqlQuery objectQuery(db);
    objectQuery.prepare("SELECT id FROM objects WHERE image_id = :image_id LIMIT 1");
    objectQuery.bindValue(":image_id", currentImageId);

    int objectId = 0;
    if (objectQuery.exec() && objectQuery.next())
        objectId = objectQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO detections (image_id, object_id, x, y, width, height, confidence) "
        "VALUES (:image_id, :object_id, :x, :y, :width, :height, :confidence)"
        );
    query.bindValue(":image_id", currentImageId);
    query.bindValue(":object_id", objectId > 0 ? QVariant(objectId) : QVariant());
    query.bindValue(":x", 120.0);
    query.bindValue(":y", 90.0);
    query.bindValue(":width", 180.0);
    query.bindValue(":height", 140.0);
    query.bindValue(":confidence", 0.85);
    query.exec();
}

void MainWindow::loadDetections()
{
    if (currentImageId < 0)
        return;

    const auto items = imageScene->items();
    for (QGraphicsItem *item : items) {
        if (auto *rectItem = dynamic_cast<DetectionRectItem *>(item))
            imageScene->removeItem(rectItem);
    }

    clearDetectionItems();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare("SELECT id, x, y, width, height FROM detections WHERE image_id = :image_id");
    query.bindValue(":image_id", currentImageId);
    if (!query.exec())
        return;

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
        ui->objectInfoEdit->setPlainText("No object selected.");
        return;
    }

    const int row = current.row();
    const QString name = objectsModel->data(objectsModel->index(row, 1)).toString();
    const QString type = objectsModel->data(objectsModel->index(row, 2)).toString();
    const QString ra = objectsModel->data(objectsModel->index(row, 4)).toString();
    const QString dec = objectsModel->data(objectsModel->index(row, 5)).toString();
    const QString mag = objectsModel->data(objectsModel->index(row, 6)).toString();
    const QString constellation = objectsModel->data(objectsModel->index(row, 7)).toString();

    ui->objectInfoEdit->setPlainText(
        QString("Object summary\n\n"
                "Name: %1\nType: %2\nRA: %3\nDec: %4\n"
                "Magnitude: %5\nConstellation: %6\n\n"
                "Draw a detection for this object or link an existing one.")
            .arg(name, type, ra, dec, mag, constellation)
        );
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
    if (objectId > 0)
        highlightDetectionForObject(objectId);

    ui->rightTabWidget->setCurrentWidget(ui->catalogTab);
    updateObjectInfo();
}

void MainWindow::handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    ui->rightTabWidget->setCurrentWidget(ui->detectionsTab);
    highlightCurrentDetection();
    updateDetectionInfo();
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
