#ifndef IMAGEANALYZER_H
#define IMAGEANALYZER_H

#include <QObject>
#include <QString>

struct SkyAnalysisRequest {
    int imageId = -1;
    QString imagePath;
    double centerRaDeg = 0.0;
    double centerDecDeg = 0.0;
    double fieldRadiusDeg = 1.0;
    int imageWidth = 0;
    int imageHeight = 0;
    bool replaceExisting = true;
};

struct SkyAnalysisResult {
    bool success = false;
    QString message;
    int objectsCreated = 0;
    int detectionsCreated = 0;
};

class ImageAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit ImageAnalyzer(QObject *parent = nullptr);

public slots:
    void analyze(const SkyAnalysisRequest &request);

signals:
    void finished(const SkyAnalysisResult &result);

private:
    static QPointF projectToPixel(double raDeg,
                                  double decDeg,
                                  double centerRaDeg,
                                  double centerDecDeg,
                                  double fieldRadiusDeg,
                                  int imageWidth,
                                  int imageHeight);
};

Q_DECLARE_METATYPE(SkyAnalysisRequest)
Q_DECLARE_METATYPE(SkyAnalysisResult)

#endif // IMAGEANALYZER_H
