#include "apptheme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

namespace AppTheme {

void applyFusionDarkPalette(QApplication *app)
{
    if (!app)
        return;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(7, 11, 20));
    palette.setColor(QPalette::WindowText, QColor(231, 238, 255));
    palette.setColor(QPalette::Base, QColor(9, 16, 29));
    palette.setColor(QPalette::AlternateBase, QColor(13, 22, 48));
    palette.setColor(QPalette::ToolTipBase, QColor(20, 28, 52));
    palette.setColor(QPalette::ToolTipText, QColor(231, 238, 255));
    palette.setColor(QPalette::Text, QColor(231, 238, 255));
    palette.setColor(QPalette::Button, QColor(21, 29, 54));
    palette.setColor(QPalette::ButtonText, QColor(234, 240, 255));
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(140, 168, 255));
    palette.setColor(QPalette::Highlight, QColor(63, 45, 123));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(112, 128, 170));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(112, 128, 170));
    app->setPalette(palette);
}

QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget {
            background: #070b14;
            color: #e7eeff;
            font-size: 13px;
        }

        QFrame#topToolbarFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0c1223, stop:0.55 #101938, stop:1 #16153a);
            border: 1px solid #2c3768;
            border-radius: 10px;
        }

        QFrame#topToolbarFrame QPushButton {
            min-height: 26px;
            padding: 3px 10px;
        }

        QFrame#toolbarSep1, QFrame#toolbarSep2, QFrame#toolbarSep3 {
            color: #3d4f82;
            max-width: 1px;
            margin: 4px 2px;
        }

        QFrame#imagePanelFrame,
        QFrame#rightPanelFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0c1223, stop:0.55 #101938, stop:1 #16153a);
            border: 1px solid #2c3768;
            border-radius: 12px;
        }

        QFrame#imagePanelHeaderFrame,
        QFrame#statsFrame {
            background: transparent;
            border: none;
        }

        QLabel#imagePanelLabel,
        QLabel#activityLogTitleLabel,
        QLabel#objectInfoTitleLabel {
            color: #8ca8ff;
            font-weight: 600;
            font-size: 12px;
            letter-spacing: 0.3px;
        }

        QLabel#canvasHintLabel {
            color: #7f91c7;
            font-size: 11px;
            padding-right: 2px;
        }

        QLabel#sceneMetaLabel {
            color: #8fa4df;
        }

        QLabel#statsImageTitleLabel,
        QLabel#statsObjectsTitleLabel,
        QLabel#statsDetectionsTitleLabel {
            color: #87a0ff;
            font-size: 11px;
            font-weight: 600;
            text-transform: uppercase;
        }

        QLabel#currentImageValueLabel,
        QLabel#objectsCountValueLabel,
        QLabel#detectionsCountValueLabel {
            color: #f0f4ff;
            font-size: 18px;
            font-weight: 700;
            padding: 2px 6px 6px 0;
        }

        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #1b2550, stop:1 #24366c);
            color: #f3f7ff;
            border: 1px solid #5f83ff;
            border-radius: 8px;
            padding: 5px 12px;
            min-height: 28px;
            font-weight: 600;
        }

        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #22306a, stop:1 #30478c);
            border: 1px solid #86b2ff;
        }

        QPushButton:pressed {
            background: #31498d;
            border: 1px solid #a8c5ff;
        }

        QPushButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #5a2f9a, stop:1 #7a46cb);
            border: 1px solid #c89fff;
            color: #fff8ff;
        }

        QPushButton:disabled {
            background: #101827;
            color: #7080aa;
            border: 1px solid #25304f;
        }

        QPushButton#openImageButton,
        QPushButton#analyzeSkyButton,
        QPushButton#validateButton,
        QPushButton#exportYoloButton,
        QPushButton#focusDetectionButton,
        QPushButton#sendChatButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #5a2f9a, stop:1 #7b47ce);
            border: 1px solid #c394ff;
            color: #fff8ff;
        }

        QPushButton#openImageButton:hover,
        QPushButton#analyzeSkyButton:hover,
        QPushButton#validateButton:hover,
        QPushButton#exportYoloButton:hover,
        QPushButton#focusDetectionButton:hover,
        QPushButton#sendChatButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #6a39b3, stop:1 #8b56dd);
            border: 1px solid #dbb8ff;
        }

        QPushButton#deleteDetectionButton,
        QPushButton#deleteInvalidDetectionButton,
        QPushButton#deleteInvalidDetectionButtonInline,
        QPushButton#deleteTypeButton,
        QPushButton#deleteObjectButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #5a1f3f, stop:1 #7a2a55);
            border: 1px solid #d06d96;
            color: #ffe7f1;
        }

        QPushButton#deleteDetectionButton:hover,
        QPushButton#deleteInvalidDetectionButton:hover,
        QPushButton#deleteInvalidDetectionButtonInline:hover,
        QPushButton#deleteTypeButton:hover,
        QPushButton#deleteObjectButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #6e274d, stop:1 #933365);
            border: 1px solid #f093b6;
        }

        QTabWidget::pane {
            border: 1px solid #2d3866;
            border-radius: 10px;
            top: -1px;
            background: #0b1020;
        }

        QTabBar::tab {
            background: #131c37;
            color: #9fb4eb;
            border: 1px solid #2e3968;
            padding: 8px 14px;
            min-width: 88px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 4px;
        }

        QTabBar::tab:selected {
            background: #1d2a54;
            color: #f4f7ff;
            border-color: #5977d9;
            font-weight: 600;
        }

        QTabBar::tab:hover:!selected {
            background: #182444;
            color: #d5e2ff;
        }

        QTableView {
            background: #09101d;
            alternate-background-color: #0d1630;
            color: #e8efff;
            border: 1px solid #2b3562;
            border-radius: 9px;
            selection-background-color: #3f2d7b;
            selection-color: #ffffff;
            outline: 0;
        }

        QTableView::item {
            padding: 6px 8px;
        }

        QTableView::item:hover {
            background: rgba(95, 131, 255, 28);
        }

        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #172346, stop:1 #121c39);
            color: #acd0ff;
            padding: 7px 8px;
            border: none;
            border-right: 1px solid #2d3766;
            border-bottom: 1px solid #2d3766;
            font-weight: 600;
        }

        QPlainTextEdit {
            background: #09101d;
            color: #dfe7ff;
            border: 1px solid #2b3562;
            border-radius: 9px;
            padding: 10px;
            selection-background-color: #4d34a0;
            selection-color: #ffffff;
            line-height: 1.35;
        }

        QPlainTextEdit#validationLogEdit,
        QPlainTextEdit#guideSummaryEdit,
        QPlainTextEdit#chatHistoryEdit {
            font-family: Menlo, Monaco, "Courier New", monospace;
            font-size: 11px;
            color: #c8d6ff;
        }

        QLineEdit#chatInputEdit {
            background: #09101d;
            color: #e8efff;
            border: 1px solid #2b3562;
            border-radius: 8px;
            padding: 6px 10px;
            selection-background-color: #4d34a0;
        }

        QSplitter::handle {
            background: #1a2340;
        }

        QSplitter::handle:horizontal {
            width: 6px;
            margin: 0 2px;
            border-radius: 3px;
        }

        QSplitter::handle:horizontal:hover {
            background: #334878;
        }

        QStatusBar {
            background: #0a1020;
            color: #b8c7f3;
            border-top: 1px solid #232d55;
            padding-left: 6px;
        }

        QScrollBar:vertical {
            background: #0d1428;
            width: 11px;
            margin: 2px;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical {
            background: #3a4f86;
            min-height: 28px;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical:hover {
            background: #5a78c8;
        }

        QScrollBar:horizontal {
            background: #0d1428;
            height: 11px;
            margin: 2px;
            border-radius: 5px;
        }

        QScrollBar::handle:horizontal {
            background: #3a4f86;
            min-width: 28px;
            border-radius: 5px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #5a78c8;
        }

        QScrollBar::add-line, QScrollBar::sub-line,
        QScrollBar::add-page, QScrollBar::sub-page {
            background: none;
            border: none;
        }

        QToolTip {
            background: #141d36;
            color: #eef3ff;
            border: 1px solid #4a5f9a;
            padding: 6px 8px;
            border-radius: 6px;
        }
    )");
}

QString imageViewStyleSheet()
{
    return QStringLiteral(R"(
        QGraphicsView {
            background: qradialgradient(cx:0.5, cy:0.45, radius:1.08, fx:0.5, fy:0.45,
                stop:0 #16203e, stop:0.48 #0d1327, stop:1 #05070c);
            border: 1px solid #2b3562;
            border-radius: 10px;
        }
    )");
}

} // namespace AppTheme
