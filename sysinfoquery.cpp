#include "sysinfoquery.h"
#include <QDebug>
#include <QThread>
#define MEM_MAX 3808168.0
extern "C" {
#include <unistd.h>

}
SysInfoQuery::SysInfoQuery(QObject *parent) : QObject(parent)
{
    timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, &QTimer::timeout, this, &SysInfoQuery::getSysInfo);

    //timer->start();
}

void SysInfoQuery::getSysInfo()
{
    double cpu = 0.0;
    long rss = 0;
    getLocalIPv4();


    cmd = "ps -p " + QString::number(getpid())
            + " -o %cpu,rss";

    if (!p) {
        p = new QProcess();
    }
    p->start("bash", QStringList() << "-c" << cmd);
    p->waitForFinished();


    QString output = p->readAllStandardOutput();
    QStringList lines = output.split('\n');
    if (lines.size() >= 2) {
        QStringList values = lines[1].trimmed().split(QRegExp("\\s+"));
        if (values.size() >= 2) {
            cpu = values[0].toDouble();
            rss = values[1].toLong();
            // qDebug() << "CPU%:" << cpu << "RSS:" << rss << "KB";
        }
    }

    double memUsage = rss / MEM_MAX;





    //    qDebug() << "ip:-----------------------:" << ip;



    p->start("cat", QStringList() << "/sys/kernel/debug/rknpu/load");

    if (!p->waitForFinished(3000)) {
        qDebug() << "命令执行超时";
        return;
    }
    QString npuUsage = p->readAllStandardOutput().trimmed();
    //    qDebug() << "原始输出:" << npuUsage;  // "NPU load:  0%"
    QStringList parts = npuUsage.split(QRegExp("\\s+"));
    QString npuUsageNumStr("");
    if (parts.size() >= 3) {
        npuUsageNumStr = parts[2].remove('%');
        //        qDebug() << "-------------------------" << numStr;
    }

    emit SysInfoReady(QString::number(cpu), QString::number(memUsage * 100, 'f', 2), npuUsageNumStr, ip, ipv4GetSuccess);
    //    qDebug() << p.readAllStandardOutput() << "-------------------------------------------------";

    //qDebug() << "SysInfo:" << QThread::currentThread();

}

QList<QHostAddress> SysInfoQuery::getLocalIPv4()
{
    QList<QHostAddress> result;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (QNetworkInterface interface, interfaces) {
        // 过滤：必须已启用、非回环、能运行（通常是有效的网络接口）
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
                !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            foreach (QNetworkAddressEntry entry, entries) {
                QHostAddress addr = entry.ip();
                // 只取 IPv4 地址
                if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                    result.append(addr);
                }
            }
        }
    }
    if (result.empty()) {
        ip = "";
        ipv4GetSuccess = false;
    } else {
        ip = result.first().toString();
        ipv4GetSuccess = true;
    }
    return result;
}

void SysInfoQuery::tiemrStart()
{
    timer->start();
}



