#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

#include "mainwindow.h"

static bool initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "objects-connection");
    db.setDatabaseName("objects.db");

    if (!db.open()) {
        qCritical() << "Cannot open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    const char *createTypesSql =
        "CREATE TABLE IF NOT EXISTS object_types ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL,"
        "    description TEXT"
        ");";

    if (!query.exec(createTypesSql)) {
        qCritical() << "Failed to create table object_types:" << query.lastError().text();
        return false;
    }

    const char *createImagesSql =
        "CREATE TABLE IF NOT EXISTS images ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    path TEXT NOT NULL,"
        "    title TEXT,"
        "    created_at TEXT"
        ");";

    if (!query.exec(createImagesSql)) {
        qCritical() << "Failed to create table images:" << query.lastError().text();
        return false;
    }

    const char *createObjectsSql =
        "CREATE TABLE IF NOT EXISTS objects ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL,"
        "    type_id INTEGER,"
        "    image_id INTEGER,"
        "    ra REAL,"
        "    dec REAL,"
        "    magnitude REAL,"
        "    constellation TEXT,"
        "    FOREIGN KEY(type_id) REFERENCES object_types(id),"
        "    FOREIGN KEY(image_id) REFERENCES images(id)"
        ");";

    if (!query.exec(createObjectsSql)) {
        qCritical() << "Failed to create table objects:" << query.lastError().text();
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!initDatabase())
        return 1;

    MainWindow w;
    w.show();
    return a.exec();
}
