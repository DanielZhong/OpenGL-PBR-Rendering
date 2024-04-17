#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *e);

private slots:
    void on_actionLoad_Environment_Map_triggered();

    void on_actionLoad_Scene_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
