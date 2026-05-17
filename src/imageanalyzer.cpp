#include "imageanalyzer.h"

#include "applogger.h"
#include "catalogclient.h"

#include <QPointF>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtMath>

#include <cmath>

namespace {

int ensureObjectTypeId(QSqlDatabase &db, const QString &typeName)
{
    if (typeName.trimmed().isEmpty())
        return 0;

    QSqlQuery find(db);
    find.prepare(QStringLiteral("SELECT id FROM object_types WHERE name = :name LIMIT 1"));
    find.bindValue(QStringLiteral(":name"), typeName.trimmed());
    if (find.exec() && find.next())
        return find.value(0).toInt();

    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT INTO object_types (name, description) VALUES (:name, :description)"));
    insert.bindValue(QStringLiteral(":name"), typeName.trimmed());
    insert.bindValue(QStringLiteral(":description"),
                     QStringLiteral("Auto-created from SIMBAD type mapping"));
    if (!insert.exec())
        return 0;

    return insert.lastInsertId().toInt();
}

QString formatRaHours(double raDeg)
{
    const double hours = raDeg / 15.0;
    return QString::number(hours, 'f', 4);
}

QString formatDecDegrees(double decDeg)
{
    return QString::number(decDeg, 'f', 4);
}

} // namespace

ImageAnalyzer::ImageAnalyzer(QObject *parent)
    : QObject(parent)
{
}

QPointF ImageAnalyzer::projectToPixel(double raDeg,
                                      double decDeg,
                                      double centerRaDeg,
                                      double centerDecDeg,
                                      double fieldRadiusDeg,
                                      int imageWidth,
                                      int imageHeight)
{
    const double cosDec = std::cos(qDegreesToRadians(centerDecDeg));
    const double deltaRa = (raDeg - centerRaDeg) * cosDec;
    const double deltaDec = decDeg - centerDecDeg;

    const double pixelsPerDeg = qMin(imageWidth, imageHeight) / (2.0 * fieldRadiusDeg);
    const double x = imageWidth * 0.5 + deltaRa * pixelsPerDeg;
    const double y = imageHeight * 0.5 - deltaDec * pixelsPerDeg;
    return QPointF(x, y);
}

void ImageAnalyzer::analyze(const SkyAnalysisRequest &request)
{
    SkyAnalysisResult result;

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("objects-connection"));
    if (!db.isValid() || !db.isOpen()) {
        result.message = QStringLiteral("Database is not available.");
        emit finished(result);
        return;
    }

    if (request.imageId < 0 || request.imageWidth <= 0 || request.imageHeight <= 0) {
        result.message = QStringLiteral("Image is not loaded.");
        emit finished(result);
        return;
    }

    QString catalogError;
    const CatalogClient catalog;
    const QList<CatalogEntry> entries = catalog.queryCone(request.centerRaDeg,
                                                          request.centerDecDeg,
                                                          request.fieldRadiusDeg,
                                                          35,
                                                          &catalogError);

    if (entries.isEmpty()) {
        result.message = catalogError.isEmpty()
                               ? QStringLiteral("No catalog objects found in this field.")
                               : catalogError;
        emit finished(result);
        return;
    }

    if (!db.transaction()) {
        result.message = QStringLiteral("Failed to start database transaction.");
        emit finished(result);
        return;
    }

    if (request.replaceExisting) {
        QSqlQuery deleteDetections(db);
        deleteDetections.prepare(QStringLiteral("DELETE FROM detections WHERE image_id = :image_id"));
        deleteDetections.bindValue(QStringLiteral(":image_id"), request.imageId);
        deleteDetections.exec();

        QSqlQuery deleteObjects(db);
        deleteObjects.prepare(QStringLiteral("DELETE FROM objects WHERE image_id = :image_id"));
        deleteObjects.bindValue(QStringLiteral(":image_id"), request.imageId);
        deleteObjects.exec();
    }

    QSqlQuery updateImage(db);
    updateImage.prepare(
        QStringLiteral("UPDATE images SET center_ra = :ra, center_dec = :dec, field_radius_deg = :radius, "
                       "analysis_status = :status, analyzed_at = datetime('now') WHERE id = :id"));
    updateImage.bindValue(QStringLiteral(":ra"), request.centerRaDeg);
    updateImage.bindValue(QStringLiteral(":dec"), request.centerDecDeg);
    updateImage.bindValue(QStringLiteral(":radius"), request.fieldRadiusDeg);
    updateImage.bindValue(QStringLiteral(":status"), QStringLiteral("catalog_matched"));
    updateImage.bindValue(QStringLiteral(":id"), request.imageId);
    updateImage.exec();

    const double boxSize = qMax(24.0, qMin(request.imageWidth, request.imageHeight) * 0.04);

    for (const CatalogEntry &entry : entries) {
        const QPointF pixel = projectToPixel(entry.raDeg,
                                             entry.decDeg,
                                             request.centerRaDeg,
                                             request.centerDecDeg,
                                             request.fieldRadiusDeg,
                                             request.imageWidth,
                                             request.imageHeight);

        if (pixel.x() < 0 || pixel.y() < 0 || pixel.x() >= request.imageWidth || pixel.y() >= request.imageHeight)
            continue;

        const int typeId = ensureObjectTypeId(db, entry.guessedType);

        QSqlQuery insertObject(db);
        insertObject.prepare(
            QStringLiteral("INSERT INTO objects (name, type_id, image_id, ra, dec, magnitude, constellation, "
                           "messier, ngc, ic, identification_status, guessed_type, simbad_type) "
                           "VALUES (:name, :type_id, :image_id, :ra, :dec, :mag, :const, "
                           ":messier, :ngc, :ic, :status, :guessed, :simbad)"));
        insertObject.bindValue(QStringLiteral(":name"), entry.mainId);
        insertObject.bindValue(QStringLiteral(":type_id"), typeId > 0 ? QVariant(typeId) : QVariant());
        insertObject.bindValue(QStringLiteral(":image_id"), request.imageId);
        insertObject.bindValue(QStringLiteral(":ra"), formatRaHours(entry.raDeg));
        insertObject.bindValue(QStringLiteral(":dec"), formatDecDegrees(entry.decDeg));
        insertObject.bindValue(QStringLiteral(":mag"),
                               entry.magnitude < 90.0 ? QString::number(entry.magnitude, 'f', 2) : QVariant());
        insertObject.bindValue(QStringLiteral(":const"), entry.constellation);
        insertObject.bindValue(QStringLiteral(":messier"), entry.messier);
        insertObject.bindValue(QStringLiteral(":ngc"), entry.ngc);
        insertObject.bindValue(QStringLiteral(":ic"), entry.ic);
        insertObject.bindValue(QStringLiteral(":status"), entry.identificationStatus);
        insertObject.bindValue(QStringLiteral(":guessed"), entry.guessedType);
        insertObject.bindValue(QStringLiteral(":simbad"), entry.simbadType);

        if (!insertObject.exec())
            continue;

        const int objectId = insertObject.lastInsertId().toInt();

        const double x = pixel.x() - boxSize * 0.5;
        const double y = pixel.y() - boxSize * 0.5;

        QSqlQuery insertDetection(db);
        insertDetection.prepare(
            QStringLiteral("INSERT INTO detections (image_id, object_id, x, y, width, height, confidence) "
                           "VALUES (:image_id, :object_id, :x, :y, :width, :height, :confidence)"));
        insertDetection.bindValue(QStringLiteral(":image_id"), request.imageId);
        insertDetection.bindValue(QStringLiteral(":object_id"), objectId);
        insertDetection.bindValue(QStringLiteral(":x"), x);
        insertDetection.bindValue(QStringLiteral(":y"), y);
        insertDetection.bindValue(QStringLiteral(":width"), boxSize);
        insertDetection.bindValue(QStringLiteral(":height"), boxSize);
        insertDetection.bindValue(QStringLiteral(":confidence"), 0.9);

        if (insertDetection.exec()) {
            ++result.detectionsCreated;
            ++result.objectsCreated;
        }
    }

    if (!db.commit()) {
        db.rollback();
        result.message = QStringLiteral("Failed to save analysis results.");
        emit finished(result);
        return;
    }

    result.success = true;
    result.message = QStringLiteral("Found %1 catalog object(s); placed %2 on the image.")
                         .arg(entries.size())
                         .arg(result.objectsCreated);

    LOG_INFO(QStringLiteral("Analysis"), result.message);
    emit finished(result);
}
