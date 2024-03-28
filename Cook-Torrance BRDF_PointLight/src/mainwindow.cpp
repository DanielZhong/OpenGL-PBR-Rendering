#include "mainwindow.h"
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->mygl->setFocus();

    connect(ui->redSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setRed(int)));
    connect(ui->greenSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setGreen(int)));
    connect(ui->blueSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setBlue(int)));

    connect(ui->metallicSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setMetallic(int)));
    connect(ui->roughnessSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setRoughness(int)));
    connect(ui->aoSlider, SIGNAL(valueChanged(int)), ui->mygl, SLOT(slot_setAO(int)));
    connect(ui->checkBox, SIGNAL(clicked(bool)), this, SLOT(on_RustToggle_clicked(bool)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionCamera_Controls_triggered()
{
    CameraControlsHelp* c = new CameraControlsHelp();
    c->show();
}

void MainWindow::on_checkBox_stateChanged(int arg1) {
    bool checked = arg1 == Qt::Checked;
    ui->mygl->slot_setRustEffect(checked);
}
