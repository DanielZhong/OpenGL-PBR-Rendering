#ifndef CONTROLS_H
#define CONTROLS_H

#include <QWidget>

namespace Ui {
class controls;
}

class controls : public QWidget
{
    Q_OBJECT

public:
    explicit controls(QWidget *parent = nullptr);
    ~controls();

private:
    Ui::controls *ui;

signals:
    void sig_ChangeSurfaceShader(int);
    void sig_ChangePostShader(int);
    void sig_ChangeMatcap(int);

public slots:
    void slot_ChangeSurfaceShader(int);
    void slot_ChangePostShader(int);
    void slot_ChangeMatcap(int);
};

#endif // CONTROLS_H
