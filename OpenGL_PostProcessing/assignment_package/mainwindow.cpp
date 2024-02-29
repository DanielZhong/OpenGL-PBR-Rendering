#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      uiControls()
{
    ui->setupUi(this);
    uiControls.show();
    uiControls.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width()*0.75, 0));

    connect(&uiControls, SIGNAL(sig_ChangeSurfaceShader(int)),
            ui->mygl, SLOT(slot_ChangeSurfaceShader(int)));
    connect(&uiControls, SIGNAL(sig_ChangePostShader(int)),
            ui->mygl, SLOT(slot_ChangePostShader(int)));
    connect(&uiControls, SIGNAL(sig_ChangeMatcap(int)),
            ui->mygl, SLOT(slot_ChangeMatcap(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    uiControls.close();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    uiControls.close();
    QMainWindow::closeEvent(e);
}
