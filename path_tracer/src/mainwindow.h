#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionQuit_triggered();

    void on_actionCamera_Controls_triggered();

    void on_actionLoad_Environment_Map_Ctrl_O_triggered();

    void on_actionLoad_JSON_Scene_triggered();

private:
    Ui::MainWindow *ui;
};


#endif // MAINWINDOW_H
