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

void Dialog::setText(QString msg)
{
    ui->label->setText(msg);
    ui->btn_back->show();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::failProcess(QString msg)
{
    ui->label->text() = msg;
    ui->btn_back->show();

}

void Dialog::on_btn_back_clicked()
{
    this->deleteLater();
    emit requestClear();
}

