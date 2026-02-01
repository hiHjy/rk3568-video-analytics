#ifndef RGAWORKER_H
#define RGAWORKER_H

#include <QObject>

class RGAWorker : public QObject
{
    Q_OBJECT
public:
    explicit RGAWorker(QObject *parent = nullptr);

signals:

};

#endif // RGAWORKER_H
