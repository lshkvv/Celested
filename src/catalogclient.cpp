#include "catalogclient.h"

#include "applogger.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

#include <cmath>

namespace {

QString urlEncodeQuery(const QString &adql)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("request"), QStringLiteral("doQuery"));
    query.addQueryItem(QStringLiteral("lang"), QStringLiteral("adql"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("query"), adql);
    return query.toString(QUrl::FullyEncoded);
}

} // namespace

QString CatalogClient::extractCatalogId(const QString &mainId, const QString &prefix)
{
    static const QRegularExpression re(QStringLiteral("(%1\\s*\\d+[A-Z]?)").arg(prefix),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(mainId);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QString CatalogClient::mapSimbadType(const QString &otype)
{
    const QString t = otype.toLower();
    if (t.contains(QStringLiteral("galaxy")) || t.contains(QStringLiteral("gin")))
        return QStringLiteral("Galaxy");
    if (t.contains(QStringLiteral("nebula")) || t.contains(QStringLiteral("hii"))
        || t.contains(QStringLiteral("pn")))
        return QStringLiteral("Nebula");
    if (t.contains(QStringLiteral("cluster")))
        return QStringLiteral("Star cluster");
    if (t.contains(QStringLiteral("star")))
        return QStringLiteral("Star");
    return QStringLiteral("Deep-sky object");
}

QString CatalogClient::guessConstellation(double raDeg, double decDeg)
{
    Q_UNUSED(raDeg)
    Q_UNUSED(decDeg)
    return QString();
}

QList<CatalogEntry> CatalogClient::queryCone(double centerRaDeg,
                                             double centerDecDeg,
                                             double radiusDeg,
                                             int maxResults,
                                             QString *errorMessage) const
{
    QList<CatalogEntry> results;

    const QString adql = QStringLiteral(
                             "SELECT TOP %1 main_id, ra, dec, otype, Vmag "
                             "FROM basic "
                             "WHERE CONTAINS(POINT('ICRS', ra, dec), "
                             "CIRCLE('ICRS', %2, %3, %4)) = 1 "
                             "AND otype NOT LIKE '%%Star%%' "
                             "ORDER BY Vmag")
                             .arg(maxResults)
                             .arg(centerRaDeg, 0, 'f', 6)
                             .arg(centerDecDeg, 0, 'f', 6)
                             .arg(radiusDeg, 0, 'f', 6);

    const QUrl url(QStringLiteral("https://simbad.cds.unistra.fr/simbad/sim-tap/sync?")
                   + urlEncodeQuery(adql));

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Celested/1.0"));

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = QStringLiteral("SIMBAD query failed: %1").arg(reply->errorString());
        LOG_ERROR(QStringLiteral("Catalog"), message);
        if (errorMessage)
            *errorMessage = message;
        reply->deleteLater();
        return results;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
  for (const QJsonValue &rowValue : data) {
        const QJsonArray row = rowValue.toArray();
        if (row.size() < 4)
            continue;

        CatalogEntry entry;
        entry.mainId = row.at(0).toString().trimmed();
        entry.raDeg = row.at(1).toDouble();
        entry.decDeg = row.at(2).toDouble();
        entry.simbadType = row.at(3).toString().trimmed();
        entry.magnitude = row.size() > 4 && !row.at(4).isNull() ? row.at(4).toDouble() : 99.0;

        entry.messier = extractCatalogId(entry.mainId, QStringLiteral("M"));
        entry.ngc = extractCatalogId(entry.mainId, QStringLiteral("NGC"));
        entry.ic = extractCatalogId(entry.mainId, QStringLiteral("IC"));
        entry.guessedType = mapSimbadType(entry.simbadType);

        if (!entry.messier.isEmpty() || !entry.ngc.isEmpty() || !entry.ic.isEmpty()) {
            entry.identificationStatus = QStringLiteral("matched");
        } else if (!entry.simbadType.isEmpty()) {
            entry.identificationStatus = QStringLiteral("guessed");
        } else {
            entry.identificationStatus = QStringLiteral("not_found");
        }

        if (!entry.mainId.isEmpty())
            results.append(entry);
    }

    LOG_INFO(QStringLiteral("Catalog"),
             QStringLiteral("SIMBAD returned %1 object(s)").arg(results.size()));
    return results;
}
