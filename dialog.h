#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    void setText(QString msg);

    ~Dialog();
signals:
    void requestClear();

public slots:
    void failProcess(QString msg);
private slots:
    void on_btn_back_clicked();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
