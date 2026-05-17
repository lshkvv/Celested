#ifndef CATALOGCLIENT_H
#define CATALOGCLIENT_H

#include <QList>
#include <QString>

struct CatalogEntry {
    QString mainId;
    QString messier;
    QString ngc;
    QString ic;
    QString simbadType;
    QString guessedType;
    QString constellation;
    double raDeg = 0.0;
    double decDeg = 0.0;
    double magnitude = 99.0;
    QString identificationStatus; // matched | guessed
};

class CatalogClient
{
public:
  QList<CatalogEntry> queryCone(double centerRaDeg,
                                double centerDecDeg,
                                double radiusDeg,
                                int maxResults = 40,
                                QString *errorMessage = nullptr) const;

private:
  static QString extractCatalogId(const QString &mainId, const QString &prefix);
  static QString mapSimbadType(const QString &otype);
  static QString guessConstellation(double raDeg, double decDeg);
};

#endif // CATALOGCLIENT_H
