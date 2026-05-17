#include "skyfielddialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

SkyFieldDialog::SkyFieldDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Field center for catalog search"));
    setModal(true);

    auto *intro = new QLabel(
        tr("Celested queries SIMBAD for known objects in this region of the sky and places them on your image. "
           "Enter the approximate center and field size of the photo. Plate solving will be added later."),
        this);
    intro->setWordWrap(true);

    auto *raSpin = new QDoubleSpinBox(this);
    raSpin->setRange(0.0, 24.0);
    raSpin->setDecimals(4);
    raSpin->setSuffix(tr(" h"));
    raSpin->setValue(12.0);

    auto *decSpin = new QDoubleSpinBox(this);
    decSpin->setRange(-90.0, 90.0);
    decSpin->setDecimals(4);
    decSpin->setSuffix(QStringLiteral("°"));
    decSpin->setValue(45.0);

    auto *fovSpin = new QDoubleSpinBox(this);
    fovSpin->setRange(1.0, 600.0);
    fovSpin->setDecimals(1);
    fovSpin->setSuffix(tr(" arcmin"));
    fovSpin->setValue(120.0);

    auto *form = new QFormLayout();
    form->addRow(tr("Center RA"), raSpin);
    form->addRow(tr("Center Dec"), decSpin);
    form->addRow(tr("Field width"), fovSpin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addLayout(form);
    layout->addWidget(buttons);

    raSpin->setObjectName(QStringLiteral("raSpin"));
    decSpin->setObjectName(QStringLiteral("decSpin"));
    fovSpin->setObjectName(QStringLiteral("fovSpin"));
}

double SkyFieldDialog::centerRaHours() const
{
    return findChild<QDoubleSpinBox *>(QStringLiteral("raSpin"))->value();
}

double SkyFieldDialog::centerDecDeg() const
{
    return findChild<QDoubleSpinBox *>(QStringLiteral("decSpin"))->value();
}

double SkyFieldDialog::fieldRadiusDeg() const
{
    const double widthArcmin = findChild<QDoubleSpinBox *>(QStringLiteral("fovSpin"))->value();
    return (widthArcmin / 60.0) * 0.5;
}
