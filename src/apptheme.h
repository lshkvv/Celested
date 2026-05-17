#ifndef APPTHEME_H
#define APPTHEME_H

#include <QString>

class QApplication;

namespace AppTheme {

QString applicationStyleSheet();
QString imageViewStyleSheet();

void applyFusionDarkPalette(QApplication *app);

} // namespace AppTheme

#endif // APPTHEME_H
