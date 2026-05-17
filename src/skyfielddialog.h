#ifndef SKYFIELDDIALOG_H
#define SKYFIELDDIALOG_H

#include <QDialog>

class QDoubleSpinBox;

class SkyFieldDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SkyFieldDialog(QWidget *parent = nullptr);

    double centerRaHours() const;
    double centerDecDeg() const;
    double fieldRadiusDeg() const;
};

#endif // SKYFIELDDIALOG_H
