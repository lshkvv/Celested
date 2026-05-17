#ifndef DBMIGRATE_H
#define DBMIGRATE_H

class QSqlDatabase;

bool migrateDatabase(QSqlDatabase &db);

#endif // DBMIGRATE_H
