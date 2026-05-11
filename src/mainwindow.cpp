#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "detectionrectitem.h"

#include <QSqlDatabase>
#include <QSqlError>
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
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QItemSelectionModel>
#include <QVariant>
#include <QPushButton>
#include <QTimer>
#include <QGraphicsItem>
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

    ui->objectInfoEdit->setReadOnly(true);
    ui->objectInfoEdit->setPlainText("Open an image to begin.");

    ui->mainSplitter->setChildrenCollapsible(false);
    ui->mainSplitter->setStretchFactor(0, 8);
    ui->mainSplitter->setStretchFactor(1, 5);

    QTimer::singleShot(0, this, [this]() {
        ui->mainSplitter->setSizes({900, 520});
    });

    ui->imageView->setMinimumWidth(420);
    ui->imageView->setScene(imageScene);

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        ui->objectInfoEdit->setPlainText("Database is not open.");
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
    ui->typesTableView->horizontalHeader()->setStretchLastSection(true);

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
    ui->objectsTableView->horizontalHeader()->setStretchLastSection(true);

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
    ui->detectionsTableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->addTypeButton,    &QPushButton::clicked, this, &MainWindow::addType);
    connect(ui->deleteTypeButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentType);

    connect(ui->addObjectButton,    &QPushButton::clicked, this, &MainWindow::addObject);
    connect(ui->deleteObjectButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentObject);

    connect(ui->deleteDetectionButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentDetection);
    connect(ui->linkDetectionButton,   &QPushButton::clicked, this, &MainWindow::linkDetectionToSelectedObject);

    connect(ui->exportJsonButton, &QPushButton::clicked, this, &MainWindow::exportAnnotationsToJson);
    connect(ui->importJsonButton, &QPushButton::clicked, this, &MainWindow::importAnnotationsFromJson);
    connect(ui->exportYoloButton, &QPushButton::clicked, this, &MainWindow::exportAnnotationsToYolo);

    connect(ui->openImageButton,    &QPushButton::clicked, this, &MainWindow::openImage);
    connect(ui->drawDetectionButton,&QPushButton::toggled, this, &MainWindow::toggleDrawMode);

    connect(ui->zoomInButton,  &QPushButton::clicked, ui->imageView, &ImageView::zoomIn);
    connect(ui->zoomOutButton, &QPushButton::clicked, ui->imageView, &ImageView::zoomOut);
    connect(ui->fitImageButton,&QPushButton::clicked, ui->imageView, &ImageView::fitSceneInView);

    connect(ui->imageView, &ImageView::detectionDrawn,
            this, &MainWindow::saveDetectionFromRect);
    connect(ui->imageView, &ImageView::temporaryPanStateChanged,
            this, &MainWindow::handleTemporaryPanState);

    connect(ui->objectsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::handleObjectSelection);

    connect(ui->detectionsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::handleDetectionSelection);

    setupShortcuts();
    statusBar()->showMessage(
        "Ready. Shortcuts: Delete — remove detection, F — fit image, +/- — zoom, Esc — exit draw mode, Space — pan."
        );
}

MainWindow::~MainWindow()
{
    delete ui;
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
        ui->imageView->fitSceneInView();
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

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
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

void MainWindow::handleTemporaryPanState(bool active)
{
    if (active)
        showStatusHint("Pan mode: hold Space and drag with mouse.");
    else
        showStatusHint("Pan mode finished.", 1000);
}

void MainWindow::addType()
{
    if (!typesModel)
        return;

    const int row = typesModel->rowCount();
    if (!typesModel->insertRow(row))
        return;

    const QModelIndex nameIndex = typesModel->index(row, 1);
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
    ui->objectsTableView->setCurrentIndex(nameIndex);
    ui->objectsTableView->edit(nameIndex);
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
    ui->imageView->fitSceneInView();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        showStatusHint("Database is not open.");
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO images (path, title, created_at) "
                  "VALUES (:path, :title, datetime('now'))");
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

    ui->objectInfoEdit->setPlainText(
        QString("Image loaded:\n%1\n\nUse mouse wheel or zoom buttons to inspect details.")
            .arg(QFileInfo(path).fileName())
        );

    showStatusHint(QString("Image loaded: %1").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::handleObjectSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    const int objectId = currentSelectedObjectId();
    if (objectId > 0)
        highlightDetectionForObject(objectId);

    updateObjectInfo();
}

void MainWindow::handleDetectionSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    highlightCurrentDetection();
    updateDetectionInfo();
}

void MainWindow::toggleDrawMode(bool checked)
{
    ui->imageView->setDrawModeEnabled(checked);

    if (checked) {
        const int objectId = currentSelectedObjectId();
        if (objectId > 0) {
            const QString objectName =
                objectsModel->data(objectsModel->index(
                                       ui->objectsTableView->currentIndex().row(), 1)).toString();
            ui->objectInfoEdit->setPlainText(
                QString("Draw mode enabled.\n\nCurrent target object: %1\nDraw a rectangle on the image.")
                    .arg(objectName)
                );
        } else {
            ui->objectInfoEdit->setPlainText(
                "Draw mode enabled.\n\nNo object selected.\n"
                "The new detection will be created without object_id."
                );
        }
        showStatusHint("Draw mode enabled.");
    } else {
        updateObjectInfo();
        showStatusHint("Draw mode disabled.");
    }
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
    updateDetectionInfo();
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
    ui->objectInfoEdit->setPlainText("Detection deleted.");
    showStatusHint("Detection deleted.");
}

void MainWindow::linkDetectionToSelectedObject()
{
    const int detectionId = currentSelectedDetectionId();
    const int objectId    = currentSelectedObjectId();

    if (detectionId < 0 || objectId < 0)
        return;

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
    query.prepare(
        "SELECT id, object_id, x, y, width, height, confidence "
        "FROM detections WHERE image_id = :image_id ORDER BY id"
        );
    query.bindValue(":image_id", currentImageId);

    if (!query.exec()) {
        showStatusHint("Failed to read detections.");
        return;
    }

    while (query.next()) {
        QJsonObject det;
        det["id"] = query.value(0).toInt();
        det["object_id"] = query.value(1).isNull()
                               ? QJsonValue::Null
                               : QJsonValue(query.value(1).toInt());
        det["x"] = query.value(2).toDouble();
        det["y"] = query.value(3).toDouble();
        det["width"] = query.value(4).toDouble();
        det["height"] = query.value(5).toDouble();
        det["confidence"] = query.value(6).toDouble();
        detectionsArray.append(det);
    }

    root["detections"] = detectionsArray;

    const QString baseName = QFileInfo(imagePath).completeBaseName();
    const QString defaultName = baseName.isEmpty()
                                    ? "annotations.json"
                                    : baseName + "_annotations.json";

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
    imageQuery.prepare("INSERT INTO images (path, title, created_at) "
                       "VALUES (:path, :title, datetime('now'))");
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
        detQuery.bindValue(":object_id",
                           det.value("object_id").isNull()
                               ? QVariant()
                               : QVariant(det.value("object_id").toInt()));
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
    ui->imageView->fitSceneInView();

    objectsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    objectsModel->select();

    detectionsModel->setFilter(QString("image_id = %1").arg(currentImageId));
    detectionsModel->select();

    loadDetections();

    ui->objectInfoEdit->setPlainText(
        QString("JSON imported.\n\nImage: %1\nDetections loaded: %2")
            .arg(QFileInfo(imagePath).fileName())
            .arg(detectionsArray.size())
        );

    showStatusHint("JSON import completed.", 3000);
}

void MainWindow::exportAnnotationsToYolo()
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

    showStatusHint(QString("YOLO exported: %1.txt").arg(baseName), 3000);
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

    ui->objectInfoEdit->setPlainText(
        QString("Detection updated.\n\nID: %1\nx=%2  y=%3\nwidth=%4  height=%5")
            .arg(detectionId)
            .arg(rect.x(), 0, 'f', 1)
            .arg(rect.y(), 0, 'f', 1)
            .arg(rect.width(), 0, 'f', 1)
            .arg(rect.height(), 0, 'f', 1)
        );

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

    QPen defaultPen(QColor(0, 255, 140));
    defaultPen.setWidth(2);
    QBrush defaultBrush(QColor(0, 255, 140, 30));

    while (query.next()) {
        const int detectionId = query.value(0).toInt();
        const double x = query.value(1).toDouble();
        const double y = query.value(2).toDouble();
        const double w = query.value(3).toDouble();
        const double h = query.value(4).toDouble();

        auto *rectItem = new DetectionRectItem(detectionId, QRectF(x, y, w, h));
        rectItem->setPen(defaultPen);
        rectItem->setBrush(defaultBrush);

        connect(rectItem, &DetectionRectItem::clicked,
                this, &MainWindow::selectDetectionById);
        connect(rectItem, &DetectionRectItem::geometryChanged,
                this, &MainWindow::updateDetectionGeometry);

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
    query.prepare(
        "SELECT id FROM detections WHERE image_id = :image_id AND object_id = :object_id LIMIT 1"
        );
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

    QPen defaultPen(QColor(0, 255, 140));
    defaultPen.setWidth(2);
    QBrush defaultBrush(QColor(0, 255, 140, 30));

    QPen activePen(QColor(255, 170, 0));
    activePen.setWidth(3);
    QBrush activeBrush(QColor(255, 170, 0, 45));

    for (auto it = detectionItemsById.begin(); it != detectionItemsById.end(); ++it) {
        if (it.value()) {
            it.value()->setPen(defaultPen);
            it.value()->setBrush(defaultBrush);
        }
    }

    if (detectionItemsById.contains(detectionId) && detectionItemsById[detectionId]) {
        detectionItemsById[detectionId]->setPen(activePen);
        detectionItemsById[detectionId]->setBrush(activeBrush);
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
