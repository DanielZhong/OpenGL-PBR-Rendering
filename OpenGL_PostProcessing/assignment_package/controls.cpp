#include "controls.h"
#include "ui_controls.h"

controls::controls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::controls)
{
    ui->setupUi(this);

    connect(ui->surfaceShaderComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_ChangeSurfaceShader(int)));
    connect(ui->postprocessShaderComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_ChangePostShader(int)));
    connect(ui->matcapComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_ChangeMatcap(int)));
}

controls::~controls()
{
    delete ui;
}

void controls::slot_ChangeSurfaceShader(int i) {
    emit sig_ChangeSurfaceShader(i);
}
void controls::slot_ChangePostShader(int i) {
    emit sig_ChangePostShader(i);
}
void controls::slot_ChangeMatcap(int i) {
    emit sig_ChangeMatcap(i);
}
