#include "playerinfo.h"
#include "ui_playerinfo.h"

PlayerInfo::PlayerInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerInfo)
{
    ui->setupUi(this);
}

PlayerInfo::~PlayerInfo()
{
    delete ui;
}

void PlayerInfo::slot_setPosText(QString s) {
    ui->posLabel->setText(s);
}

void PlayerInfo::slot_setVelText(QString s) {
    ui->velLabel->setText(s);
}

void PlayerInfo::slot_setAccText(QString s) {
    ui->accLabel->setText(s);
}

void PlayerInfo::slot_setLookText(QString s) {
    ui->lookLabel->setText(s);
}

void PlayerInfo::slot_setChunkText(QString s) {
    ui->chunkLabel->setText(s);
}
void PlayerInfo::slot_setZoneText(QString s) {
    ui->zoneLabel->setText(s);
}

