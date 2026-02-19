#include "dialog.h"
#include "ui_dialog.h"
#include <QDebug>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    qDebug() << "dialog";
    ui->btn_back->hide();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::failProcess()
{
    ui->label->text() = "出现错误";
    ui->btn_back->show();

}

void Dialog::on_btn_back_clicked()
{
    this->deleteLater();
}

