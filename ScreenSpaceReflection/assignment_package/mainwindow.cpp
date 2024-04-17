#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QMainWindow::closeEvent(e);
}

void MainWindow::on_actionLoad_Environment_Map_triggered()
{
//    ui->mygl->loadEnvMap();
}


void MainWindow::on_actionLoad_Scene_triggered()
{
//    ui->mygl->loadScene();
}

