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

    connect(ui->addTypeButton, &QPushButton::clicked, this, &MainWindow::addType);
    connect(ui->deleteTypeButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentType);

    connect(ui->addObjectButton, &QPushButton::clicked, this, &MainWindow::addObject);
    connect(ui->deleteObjectButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentObject);

    connect(ui->deleteDetectionButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentDetection);
    connect(ui->linkDetectionButton, &QPushButton::clicked, this, &MainWindow::linkDetectionToSelectedObject);

    connect(ui->openImageButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(ui->drawDetectionButton, &QPushButton::toggled, this, &MainWindow::toggleDrawMode);

    connect(ui->zoomInButton, &QPushButton::clicked, ui->imageView, &ImageView::zoomIn);
    connect(ui->zoomOutButton, &QPushButton::clicked, ui->imageView, &ImageView::zoomOut);
    connect(ui->fitImageButton, &QPushButton::clicked, ui->imageView, &ImageView::fitSceneInView);

    connect(ui->imageView, &ImageView::detectionDrawn, this, &MainWindow::saveDetectionFromRect);

    connect(ui->objectsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &MainWindow::handleObjectSelection);

    connect(ui->detectionsTableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &MainWindow::handleDetectionSelection);
}

MainWindow::~MainWindow()
{
    delete ui;
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
        ui->objectInfoEdit->setPlainText("Failed to load image.");
        return;
    }

    imageScene->clear();
    detectionItemsById.clear();

    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    ui->imageView->fitSceneInView();

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        ui->objectInfoEdit->setPlainText("Database is not open.");
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO images (path, title, created_at) "
                  "VALUES (:path, :title, datetime('now'))");
    query.bindValue(":path", path);
    query.bindValue(":title", QFileInfo(path).fileName());

    if (!query.exec()) {
        ui->objectInfoEdit->setPlainText("Failed to save image record.");
        currentImageId = -1;
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
    } else {
        updateObjectInfo();
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

    if (objectId > 0)
        query.bindValue(":object_id", objectId);
    else
        query.bindValue(":object_id", QVariant());

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
}

void MainWindow::linkDetectionToSelectedObject()
{
    const int detectionId = currentSelectedDetectionId();
    const int objectId = currentSelectedObjectId();

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
        QString("Detection updated.\n\nDetection ID: %1\nx=%2\ny=%3\nwidth=%4\nheight=%5")
            .arg(detectionId)
            .arg(rect.x())
            .arg(rect.y())
            .arg(rect.width())
            .arg(rect.height())
        );
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

    if (!checkQuery.exec())
        return;

    if (checkQuery.next() && checkQuery.value(0).toInt() > 0)
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

    if (objectId > 0)
        query.bindValue(":object_id", objectId);
    else
        query.bindValue(":object_id", QVariant());

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
        auto *rectItem = dynamic_cast<DetectionRectItem *>(item);
        if (rectItem)
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

    QPen pen(QColor(0, 255, 140));
    pen.setWidth(2);
    QBrush brush(QColor(0, 255, 140, 30));

    while (query.next()) {
        const int detectionId = query.value(0).toInt();
        const double x = query.value(1).toDouble();
        const double y = query.value(2).toDouble();
        const double w = query.value(3).toDouble();
        const double h = query.value(4).toDouble();

        auto *rectItem = new DetectionRectItem(detectionId, QRectF(x, y, w, h));
        rectItem->setPen(pen);
        rectItem->setBrush(brush);

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

    const int detectionId = query.value(0).toInt();
    selectDetectionById(detectionId);
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

    QString text;
    text += "Object summary\n\n";
    text += "Name: " + name + "\n";
    text += "Type: " + type + "\n";
    text += "RA: " + ra + "\n";
    text += "Dec: " + dec + "\n";
    text += "Magnitude: " + mag + "\n";
    text += "Constellation: " + constellation + "\n\n";
    text += "You can draw a new detection for this object or link an existing detection to it.";

    ui->objectInfoEdit->setPlainText(text);
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

    QString text;
    text += "Detection summary\n\n";
    text += "Detection ID: " + detectionId + "\n";
    text += "Object ID: " + objectId + "\n";
    text += "X: " + x + "\n";
    text += "Y: " + y + "\n";
    text += "Width: " + w + "\n";
    text += "Height: " + h + "\n";
    text += "Confidence: " + confidence + "\n\n";
    text += "You can click, move and resize this rectangle directly on the image.";

    ui->objectInfoEdit->setPlainText(text);
}
