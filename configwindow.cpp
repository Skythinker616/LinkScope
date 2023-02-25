#include "configwindow.h"
#include "ui_configwindow.h"
#include <qdebug.h>

ConfigWindow::ConfigWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigWindow)
{
    ui->setupUi(this);
    setWindowTitle("高级设置");
}

ConfigWindow::~ConfigWindow()
{
    delete ui;
}

void ConfigWindow::setParam(ConfigWindowParam &param)
{
    ui->sb_baudrate->setValue(param.baudrate);
    ui->cb_databit->setCurrentText(QString("%1").arg(param.databits));
    ui->cb_stopbit->setCurrentText(param.stopbits==1 ? "1" : (param.stopbits==2?"2":"1.5"));
    ui->cb_parity->setCurrentText(param.parity==0 ? "无" : (param.parity==2?"偶校验":"奇校验"));
    ui->cb_sample_freq->setCurrentText(QString("%1Hz").arg(param.sampleFreq));
    ui->sb_gdb_port->setValue(param.gdbPort);
    ui->et_gdb_param->setText(param.gdbParam);
    ui->et_ocd_param->setText(param.ocdParam);
}

void ConfigWindow::getParam(ConfigWindowParam &param)
{
    param.baudrate=ui->sb_baudrate->value();
    param.databits=(QSerialPort::DataBits)ui->cb_databit->currentText().toInt();
    param.stopbits=(QSerialPort::StopBits)QList<int>{1,3,2}.at(ui->cb_stopbit->currentIndex());
    param.parity=(QSerialPort::Parity)QList<int>{0,2,3}.at(ui->cb_parity->currentIndex());
    param.sampleFreq=QList<int>{100,50,20,10,5,2,1}.at(ui->cb_sample_freq->currentIndex());
    param.gdbPort=ui->sb_gdb_port->value();
    param.gdbParam=ui->et_gdb_param->text();
    param.ocdParam=ui->et_ocd_param->text();
}

void ConfigWindow::on_bt_ok_clicked()
{
    accept();
}

void ConfigWindow::on_bt_cancel_clicked()
{
    close();
}
