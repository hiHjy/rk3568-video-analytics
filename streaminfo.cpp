#include "streaminfo.h"
#include "ui_streaminfo.h"
#include <QDebug>
StreamInfo::StreamInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StreamInfo)
{
    ui->setupUi(this);
}

StreamInfo::~StreamInfo()
{
    delete ui;
}

void StreamInfo::on_btn_disconn_clicked()
{
    emit requsetDisconn();
}

void StreamInfo::display(QString streamType, QString url)
{
    qDebug() << url << "[stream info]";
    ui->lb_type->setText(streamType);
    ui->lb_addr->setText(url);
}

