#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QSqlRelation>
#include <QDebug>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , typesModel(nullptr)
    , objectsModel(nullptr)
    , imageScene(new QGraphicsScene(this))
    , currentImageId(-1)
{
    ui->setupUi(this);

    // Image tab
    ui->imageView->setScene(imageScene);
    ui->imageView->setDragMode(QGraphicsView::ScrollHandDrag);

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        qCritical() << "Database is not open in MainWindow";
        return;
    }

    // Types
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

    connect(ui->addTypeButton, &QPushButton::clicked,
            this, &MainWindow::addType);
    connect(ui->deleteTypeButton, &QPushButton::clicked,
            this, &MainWindow::deleteCurrentType);

    // Objects (objects.image_id -> images.id, objects.type_id -> object_types.id)
    objectsModel = new QSqlRelationalTableModel(this, db);
    objectsModel->setTable("objects");
    objectsModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    objectsModel->setRelation(2, QSqlRelation("object_types", "id", "name")); // type_id
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
    ui->objectsTableView->setColumnHidden(0, true); // id
    ui->objectsTableView->setColumnHidden(3, true); // image_id
    ui->objectsTableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->addObjectButton, &QPushButton::clicked,
            this, &MainWindow::addObject);
    connect(ui->deleteObjectButton, &QPushButton::clicked,
            this, &MainWindow::deleteCurrentObject);

    // Image tab button
    connect(ui->openImageButton, &QPushButton::clicked,
            this, &MainWindow::openImage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addType()
{
    if (!typesModel)
        return;

    int row = typesModel->rowCount();
    if (!typesModel->insertRow(row))
        return;

    QModelIndex nameIndex = typesModel->index(row, 1);
    ui->typesTableView->setCurrentIndex(nameIndex);
    ui->typesTableView->edit(nameIndex);
}

void MainWindow::deleteCurrentType()
{
    if (!typesModel)
        return;

    QModelIndex index = ui->typesTableView->currentIndex();
    if (!index.isValid())
        return;

    typesModel->removeRow(index.row());
}

void MainWindow::addObject()
{
    if (!objectsModel)
        return;

    if (currentImageId < 0) {
        qWarning() << "No current image, cannot add object";
        return;
    }

    int row = objectsModel->rowCount();
    if (!objectsModel->insertRow(row))
        return;

    // колонки: 0:id, 1:name, 2:type_id, 3:image_id, 4:ra, 5:dec, 6:magnitude, 7:constellation
    objectsModel->setData(objectsModel->index(row, 3), currentImageId);

    QModelIndex nameIndex = objectsModel->index(row, 1);
    ui->objectsTableView->setCurrentIndex(nameIndex);
    ui->objectsTableView->edit(nameIndex);
}

void MainWindow::deleteCurrentObject()
{
    if (!objectsModel)
        return;

    QModelIndex index = ui->objectsTableView->currentIndex();
    if (!index.isValid())
        return;

    objectsModel->removeRow(index.row());
}

void MainWindow::openImage()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open image"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)")
        );

    if (path.isEmpty())
        return;

    QPixmap pix(path);
    if (pix.isNull()) {
        qWarning() << "Failed to load image:" << path;
        return;
    }

    imageScene->clear();
    imageScene->addPixmap(pix);
    imageScene->setSceneRect(pix.rect());
    ui->imageView->fitInView(imageScene->sceneRect(), Qt::KeepAspectRatio);

    QSqlDatabase db = QSqlDatabase::database("objects-connection");
    if (!db.isValid() || !db.isOpen()) {
        qCritical() << "Database is not open in openImage";
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO images (path, title, created_at) "
                  "VALUES (:path, :title, datetime('now'))");
    query.bindValue(":path", path);
    query.bindValue(":title", QFileInfo(path).fileName());

    if (!query.exec()) {
        qCritical() << "Failed to insert image:" << query.lastError().text();
        currentImageId = -1;
        return;
    }

    currentImageId = query.lastInsertId().toInt();

    if (objectsModel) {
        objectsModel->setFilter(QString("image_id = %1").arg(currentImageId));
        objectsModel->select();
    }
}
