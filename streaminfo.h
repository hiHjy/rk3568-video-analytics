#ifndef STREAMINFO_H
#define STREAMINFO_H

#include <QWidget>

namespace Ui {
class StreamInfo;
}

class StreamInfo : public QWidget
{
    Q_OBJECT

public:
    explicit StreamInfo(QWidget *parent = nullptr);
    ~StreamInfo();
signals:
    void requsetDisconn();
public slots:
    void on_btn_disconn_clicked();
     void display(QString streamType, QString url);
private:
    Ui::StreamInfo *ui;
};

#endif // STREAMINFO_H
