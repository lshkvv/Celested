#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addType();
    void deleteCurrentType();
    void addObject();
    void deleteCurrentObject();
    void openImage();

private:
    Ui::MainWindow *ui;

    QSqlTableModel *typesModel;
    QSqlRelationalTableModel *objectsModel;

    QGraphicsScene *imageScene;
    int currentImageId;
};

#endif // MAINWINDOW_H
