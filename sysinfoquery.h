#ifndef SYSINFOQUERY_H
#define SYSINFOQUERY_H
#include <QProcess>
#include <QObject>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostAddress>
class SysInfoQuery:public QObject
{
    Q_OBJECT
public:
    SysInfoQuery(QObject *parent = nullptr);
    QString cmd{""};
    void getSysInfo();
    QList<QHostAddress> getLocalIPv4();
    QProcess *p = nullptr;
    bool ipv4GetSuccess = false;
    QTimer *timer = nullptr;
    QString ip = "";

signals:
    void SysInfoReady(QString cpuUsage, QString memUsage, QString npuUsage, QString ip, bool ipv4GetSuccess);
public slots:
    void tiemrStart();
};

#endif // SYSINFOQUERY_H
