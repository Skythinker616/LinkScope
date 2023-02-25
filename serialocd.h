#ifndef SERIALOCD_H
#define SERIALOCD_H

#define DEBUG_FRAME_HEADER 0xDB

#include <QObject>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <QRegExp>
#include <qserialport.h>
#include <qserialportinfo.h>
#include <qthread.h>
#include <QTime>
#include <qtimer.h>

struct SerialParam {
    QString name;
    int baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::StopBits stopBits;
    QSerialPort::Parity parity;
};

class SerialOCD : public QThread
{
    Q_OBJECT
public:
    explicit SerialOCD(QObject *parent = nullptr);
    QStringList getSerialList();
    void startConnect(const SerialParam &param,int port);
    void stopConnect();

signals:
    void onErrorOccur(const QString &info);
    void onStopConnect();

private slots:
    void slotSerialReadyRead();
    void slotSocketReadyRead();

private:
    enum SerialCMD{
        SerialCMD_ReadMem,
        SerialCMD_WriteMem,
        SerialCMD_Reset
    };
    QTcpServer *server;
    QTcpSocket *socket;
    QSerialPort *port;
    QString serialName;
    SerialParam serialParam;
    int listenPort;
    QByteArray serialBuf;
    QTimer *waitReadMemTimer;
    void run();
    bool startServer(int port);
    void stopServer();
    void sendToClient(const QString &data);
    bool startSerial(const SerialParam &param);
    void stopSerial();
    void parseSerial();
    void sendSerialReadMem(int addr,int len);
    void sendSerialWriteMem(int addr,const QByteArray &data);
    void sendSerialReset();
};

#endif // SERIALOCD_H
