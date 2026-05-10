#include "mainwindow.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

static bool initializeDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "objects-connection");
    db.setDatabaseName("celested.sqlite");

    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS object_types ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL UNIQUE,"
            "description TEXT"
            ")")) {
        qCritical() << query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS images ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT NOT NULL,"
            "title TEXT,"
            "created_at TEXT"
            ")")) {
        qCritical() << query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS objects ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "type_id INTEGER,"
            "image_id INTEGER,"
            "ra TEXT,"
            "dec TEXT,"
            "magnitude REAL,"
            "constellation TEXT,"
            "FOREIGN KEY(type_id) REFERENCES object_types(id),"
            "FOREIGN KEY(image_id) REFERENCES images(id)"
            ")")) {
        qCritical() << query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS detections ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "image_id INTEGER NOT NULL,"
            "object_id INTEGER,"
            "x REAL NOT NULL,"
            "y REAL NOT NULL,"
            "width REAL NOT NULL,"
            "height REAL NOT NULL,"
            "confidence REAL,"
            "FOREIGN KEY(image_id) REFERENCES images(id),"
            "FOREIGN KEY(object_id) REFERENCES objects(id)"
            ")")) {
        qCritical() << query.lastError().text();
        return false;
    }

    query.exec("INSERT OR IGNORE INTO object_types (id, name, description) VALUES (1, 'Galaxy', 'Galaxy object')");
    query.exec("INSERT OR IGNORE INTO object_types (id, name, description) VALUES (2, 'Nebula', 'Nebula object')");
    query.exec("INSERT OR IGNORE INTO object_types (id, name, description) VALUES (3, 'Star', 'Star object')");

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!initializeDatabase())
        return -1;

    MainWindow w;
    w.show();
    return a.exec();
}
