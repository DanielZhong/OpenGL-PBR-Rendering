#ifndef PLAYERINFO_H
#define PLAYERINFO_H

#include <QWidget>

namespace Ui {
class PlayerInfo;
}

class PlayerInfo : public QWidget {
    Q_OBJECT
public:
    explicit PlayerInfo(QWidget *parent = nullptr);
    ~PlayerInfo();

public slots:
    void slot_setPosText(QString);
    void slot_setVelText(QString);
    void slot_setAccText(QString);
    void slot_setLookText(QString);
    void slot_setChunkText(QString);
    void slot_setZoneText(QString);

private:
    Ui::PlayerInfo *ui;
};

#endif // PLAYERINFO_H
