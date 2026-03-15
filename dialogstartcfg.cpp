#include "dialogstartcfg.h"
#include "ui_dialogstartcfg.h"

DialogStartCfg::DialogStartCfg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogStartCfg)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

DialogStartCfg::~DialogStartCfg()
{
    delete ui;
}
