#ifndef DIALOGSTARTCFG_H
#define DIALOGSTARTCFG_H

#include <QDialog>

namespace Ui {
class DialogStartCfg;
}

class DialogStartCfg : public QDialog
{
    Q_OBJECT

public:
    explicit DialogStartCfg(QWidget *parent = nullptr);
    ~DialogStartCfg();

private:
    Ui::DialogStartCfg *ui;
};

#endif // DIALOGSTARTCFG_H
