#include "dbmigrate.h"

#include "applogger.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <tuple>

namespace {

bool columnExists(QSqlDatabase &db, const QString &table, const QString &column)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
        return false;

    while (query.next()) {
        if (query.value(1).toString().compare(column, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

bool addColumn(QSqlDatabase &db, const QString &table, const QString &column, const QString &sqlType)
{
    if (columnExists(db, table, column))
        return true;

    QSqlQuery query(db);
    const QString sql = QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, sqlType);
    if (!query.exec(sql)) {
        LOG_ERROR(QStringLiteral("Database"),
                  QStringLiteral("Migration failed (%1): %2").arg(sql, query.lastError().text()));
        return false;
    }

    LOG_INFO(QStringLiteral("Database"), QStringLiteral("Added column %1.%2").arg(table, column));
    return true;
}

} // namespace

bool migrateDatabase(QSqlDatabase &db)
{
    if (!db.isOpen())
        return false;

    const QList<std::tuple<QString, QString, QString>> columns = {
        {QStringLiteral("images"), QStringLiteral("center_ra"), QStringLiteral("REAL")},
        {QStringLiteral("images"), QStringLiteral("center_dec"), QStringLiteral("REAL")},
        {QStringLiteral("images"), QStringLiteral("field_radius_deg"), QStringLiteral("REAL")},
        {QStringLiteral("images"), QStringLiteral("analysis_status"), QStringLiteral("TEXT")},
        {QStringLiteral("images"), QStringLiteral("analyzed_at"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("messier"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("ngc"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("ic"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("identification_status"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("guessed_type"), QStringLiteral("TEXT")},
        {QStringLiteral("objects"), QStringLiteral("simbad_type"), QStringLiteral("TEXT")},
    };

    for (const auto &entry : columns) {
        if (!addColumn(db,
                       std::get<0>(entry),
                       std::get<1>(entry),
                       std::get<2>(entry)))
            return false;
    }

    return true;
}
