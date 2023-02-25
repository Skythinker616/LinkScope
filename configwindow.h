#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QDialog>
#include <QSerialPort>

namespace Ui {
class ConfigWindow;
}

struct ConfigWindowParam {
    qint32 baudrate;
    QSerialPort::DataBits databits;
    QSerialPort::StopBits stopbits;
    QSerialPort::Parity parity;
    int sampleFreq;
    int gdbPort;
    QString gdbParam;
    QString ocdParam;
};

class ConfigWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);
    ~ConfigWindow();
    void setParam(ConfigWindowParam &param);
    void getParam(ConfigWindowParam &param);

private slots:
    void on_bt_ok_clicked();
    void on_bt_cancel_clicked();

private:
    Ui::ConfigWindow *ui;
};

#endif // CONFIGWINDOW_H
