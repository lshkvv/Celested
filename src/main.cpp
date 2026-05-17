#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStyleFactory>
#include <QCoreApplication>
#include <QDir>

#include "applogger.h"
#include "apptheme.h"
#include "dbmigrate.h"
#include "mainwindow.h"

static bool initializeDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("objects-connection"));
    db.setDatabaseName(QCoreApplication::applicationDirPath() + QStringLiteral("/objects.db"));

    if (!db.open()) {
        const QString message = QStringLiteral("Failed to open database: %1").arg(db.lastError().text());
        LOG_ERROR(QStringLiteral("Database"), message);
        QMessageBox::critical(nullptr, QStringLiteral("Database error"), message);
        return false;
    }

    LOG_INFO(QStringLiteral("Database"), QStringLiteral("Opened database at %1").arg(db.databaseName()));

    QFile schemaFile(QCoreApplication::applicationDirPath() + QStringLiteral("/schema.sql"));
    if (!schemaFile.exists())
        schemaFile.setFileName(QDir::currentPath() + QStringLiteral("/schema.sql"));

    if (!schemaFile.exists()) {
        const QString message = QStringLiteral("schema.sql not found.\nChecked:\n%1\n%2")
                                    .arg(QCoreApplication::applicationDirPath() + QStringLiteral("/schema.sql"))
                                    .arg(QDir::currentPath() + QStringLiteral("/schema.sql"));
        LOG_ERROR(QStringLiteral("Database"), message);
        QMessageBox::critical(nullptr, QStringLiteral("Database error"), message);
        return false;
    }

    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QStringLiteral("Database"), QStringLiteral("Failed to open schema.sql"));
        QMessageBox::critical(nullptr, QStringLiteral("Database error"), QStringLiteral("Failed to open schema.sql."));
        return false;
    }

    const QString schema = QString::fromUtf8(schemaFile.readAll());
    schemaFile.close();

    const QStringList statements = schema.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    for (QString statement : statements) {
        statement = statement.trimmed();
        if (statement.isEmpty())
            continue;

        QSqlQuery query(db);
        if (!query.exec(statement)) {
            const QString message = QStringLiteral("Failed to execute schema statement:\n%1\n\n%2")
                                      .arg(statement, query.lastError().text());
            LOG_ERROR(QStringLiteral("Database"), message);
            QMessageBox::critical(nullptr, QStringLiteral("Database error"), message);
            return false;
        }
    }

    LOG_INFO(QStringLiteral("Database"), QStringLiteral("Schema applied successfully."));

    if (!migrateDatabase(db)) {
        QMessageBox::critical(nullptr,
                             QStringLiteral("Database error"),
                             QStringLiteral("Failed to apply database migrations."));
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Celested"));
    app.setOrganizationName(QStringLiteral("Celested"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    AppLogger::instance().initialize();

#ifdef Q_OS_MAC
    if (QStyle *style = QStyleFactory::create(QStringLiteral("Fusion")))
        app.setStyle(style);
#endif

    AppTheme::applyFusionDarkPalette(&app);

    if (!initializeDatabase()) {
        AppLogger::instance().shutdown();
        return 1;
    }

    MainWindow w;
    w.resize(1320, 860);
    w.show();

    LOG_INFO(QStringLiteral("App"), QStringLiteral("Main window shown."));
    const int code = app.exec();

    LOG_INFO(QStringLiteral("App"), QStringLiteral("Application exiting with code %1.").arg(code));
    AppLogger::instance().shutdown();
    return code;
}
