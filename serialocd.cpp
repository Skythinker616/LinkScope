#include "serialocd.h"

SerialOCD::SerialOCD(QObject *parent) : QThread(parent)
{

}

//启动OCD（由主线程调用）
void SerialOCD::startConnect(const QString &serial, int port)
{
    serialName=serial;//保存串口号和端口号后启动子线程
    listenPort=port;
    start();//启动线程
}

//停止OCD(由主线程调用）
void SerialOCD::stopConnect()
{
    emit onStopConnect();//发送停止信号
    QTimer *exitTimer=new QTimer();//定时500ms退出子线程，保证事件循环关闭tcp和串口
    exitTimer->singleShot(500,this,&SerialOCD::quit);
    connect(exitTimer,&QTimer::timeout,exitTimer,&QTimer::deleteLater);
}

//子线程函数
void SerialOCD::run()
{
    if(startSerial(serialName) && startServer(listenPort))//串口和tcp都连接成功才进入事件循环
        exec();//进行事件循环
}

//启动tcp服务器，返回是否成功
bool SerialOCD::startServer(int port)
{
    server=new QTcpServer();//创建服务器对象
    if(server->listen(QHostAddress::Any,port))
    {
        connect(server,&QTcpServer::newConnection,[=]{//连接槽函数，在有新连接时进入
            socket=server->nextPendingConnection();//获取套接字
            connect(socket,SIGNAL(readyRead()),this,SLOT(slotSocketReadyRead()),Qt::DirectConnection);
        });
        //退出时关闭服务器
        connect(this,&SerialOCD::onStopConnect,server,&QTcpServer::close,Qt::QueuedConnection);
        connect(this,&SerialOCD::onStopConnect,server,&QTcpServer::deleteLater,Qt::QueuedConnection);
    }
    else
    {
        emit onErrorOccur(QString("端口%1监听失败").arg(port));
        server->deleteLater();
        return false;
    }
    return true;
}

//停止tcp服务器
void SerialOCD::stopServer()
{
    if(server)
    {
        if(server->isListening())
            server->close();//关闭服务器
        server->deleteLater();//销毁服务器对象
    }
}

//tcp接收槽函数
void SerialOCD::slotSocketReadyRead()
{
    //固定指令回复表，若匹配则直接回复
    const QList<QList<QString>> fixReplyTable={
        {"qSupported","PacketSize=30"},
        {"H","OK"},
        {"?","S05"},
        {"qC","QC0"},
        {"qAttached","1"},
        {"g","00000000"},
        {"p","00"},
        {"qOffsets","TextSeg=0"},
        {"qTStatus","T1"},
        {"qSymbol","OK"},
        {"m0,1","00"}
    };

    QString input=socket->readAll();//获取接收数据
    QRegExp inputRx("\\$([^#]+)#");//正则表达式截取指令部分
    inputRx.indexIn(input);
    input=inputRx.cap(1);

    if(input.isEmpty())//若截取失败则直接退出
        return;

    for(int i=0;i<fixReplyTable.length();i++)//依次在回复表中匹配
    {
        const QList<QString> &pair=fixReplyTable.at(i);
        if(input.startsWith(pair.at(0)))//若匹配成功则直接返回表中字符串
        {
            sendToClient(pair.at(1));
            return;
        }
    }

    if(input.startsWith("m"))//读取内存指令
    {
        QRegExp rx("m([0-9a-f]+),([0-9a-f]+)");//正则截取地址和字节数量
        rx.indexIn(input);
        sendSerialReadMem(rx.cap(1).toUInt(NULL,16),rx.cap(2).toUInt(NULL,16));//发送串口指令
    }
    else if(input.startsWith("M"))//写入内存指令
    {
        QRegExp rx("M([0-9a-f]+),[0-9a-f]+:([0-9a-f]+)");//正则截取地址和要写入的内容
        rx.indexIn(input);
        QString rawData=rx.cap(2);
        QByteArray data;//将截取到的内容转为qbytearray
        for(int i=0;i<rawData.length()/2;i++)
            data.append(rawData.mid(i*2,2).toUInt(NULL,16));
        sendSerialWriteMem(rx.cap(1).toUInt(NULL,16),data);//发送串口指令
        sendToClient("OK");//向gdb回复OK
    }
    else if(input.startsWith("qRcmd,7265736574"))
    {
        sendSerialReset();//发送串口指令
        sendToClient("OK");//向gdb回复OK
    }
    else//其他未知指令，回复空字符串
    {
        sendToClient("");
    }
}

//向tcp客户端（gdb）发送字符串
void SerialOCD::sendToClient(const QString &data)
{
    QByteArray array=data.toLatin1();
    int sum=0;//进行校验和计算
    for(int i=0;i<array.length();i++)
        sum+=array.at(i);
    sum%=0x100;
    QString output=QString("+$%1#%2").arg(data).arg(sum,2,16,QLatin1Char('0'));//拼接指令字符串
    socket->write(output.toStdString().c_str(),output.length());//用当前socket发送
}

//获取当前串口列表
QStringList SerialOCD::getSerialList()
{
    QStringList res;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts()) //遍历并添加获取到的串口名称
        res<<info.portName();
    return res;
}

//启动串口连接，返回是否连接成功
bool SerialOCD::startSerial(const QString &name)
{
    waitReadMemTimer=new QTimer();//创建读内存超时定时器
    waitReadMemTimer->setSingleShot(true);//仅运行一次
    waitReadMemTimer->setInterval(1000);//超时设定为1s
    connect(waitReadMemTimer,&QTimer::timeout,[=]{
        sendToClient("00000000");
        emit onErrorOccur("内存数据读取超时");
    });

    port=new QSerialPort();//创建串口对象
    port->setPortName(name);//设定端口号
    if(port->open(QIODevice::ReadWrite))//打开串口
    {
        port->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections); //波特率115200
        port->setDataBits(QSerialPort::Data8); //8数据位
        port->setFlowControl(QSerialPort::NoFlowControl);
        port->setParity(QSerialPort::NoParity); //无校验位
        port->setStopBits(QSerialPort::OneStop); //1停止位
        connect(port,SIGNAL(readyRead()),this,SLOT(slotSerialReadyRead()),Qt::DirectConnection);
        //退出时断开串口
        connect(this,&SerialOCD::onStopConnect,port,&QSerialPort::close,Qt::QueuedConnection);
        connect(this,&SerialOCD::onStopConnect,port,&QSerialPort::deleteLater,Qt::QueuedConnection);
    }
    else
    {
        emit onErrorOccur("打开串口失败");
        waitReadMemTimer->deleteLater();
        port->deleteLater();
        return false;
    }
    return true;
}

//断开串口
void SerialOCD::stopSerial()
{
    waitReadMemTimer->deleteLater();
    port->clear();//清空串口数据
    port->close();//断开串口
    port->deleteLater();//销毁串口对象
}

//串口接收槽函数
void SerialOCD::slotSerialReadyRead()
{
    serialBuf.append(port->readAll());//读出数据追加到缓存区内
    parseSerial();//解析缓存区数据
}

//解析串口缓存区数据
void SerialOCD::parseSerial()
{
    if(serialBuf[0]==(char)DEBUG_FRAME_HEADER)//第一个字节是帧头，可以进入解析
    {
        if(serialBuf.size()>2 && serialBuf.size()>=serialBuf[1])//缓存区内数据长度足够
        {
            char cmd=serialBuf[2];//命令码
            int frameLen=serialBuf[1];//帧长
            if(cmd==SerialCMD_ReadMem)//返回的是下位机读取的内存数据
            {
                waitReadMemTimer->stop();
                QString res="";
                int dataLen=frameLen-3;
                for(int i=0;i<dataLen;i++)//拼接结果字符串，返回给gdb客户端
                    res+=QString("%1").arg((unsigned char)serialBuf[i+3],2,16,QLatin1Char('0'));
                sendToClient(res);
            }
            serialBuf.remove(0,frameLen);//从缓存区中移除当前帧
            if(serialBuf.size()>0)//若缓冲区仍有数据说明后面还有数据帧，递归解析
                parseSerial();
        }
    }
    else//帧错误或不完整
    {
        while(serialBuf[0]!=(char)DEBUG_FRAME_HEADER && serialBuf.size()>0)//去除错误数据
            serialBuf.remove(0,1);
        if(serialBuf.size()>0)//若缓冲区仍有数据说明后面还有数据帧，递归解析
            parseSerial();
    }
}

//发送串口指令，请求下位机发送指定地址处指定长度的内存数据
void SerialOCD::sendSerialReadMem(int addr, int len)
{
    if(port->isOpen())
    {
        if(len>8)//最多一次性读取8个字节
            len=8;
        QByteArray arr;
        arr.resize(8);
        arr[0]=DEBUG_FRAME_HEADER;
        arr[1]=8;
        arr[2]=SerialCMD_ReadMem;
        arr[3]=len;
        for(int i=0;i<4;i++)//将地址拆分为四个字节
            arr[i+4]=(addr>>(i*8))&0xFF;
        port->write(arr);//串口发送
        waitReadMemTimer->start();
    }
}

//发送串口指令，向下位机指定地址处写入指定的内存数据
void SerialOCD::sendSerialWriteMem(int addr, const QByteArray &data)
{
    if(port->isOpen())
    {
        QByteArray arr;
        int frameLen=7+data.length();//计算总帧长
        arr.resize(frameLen);
        arr[0]=DEBUG_FRAME_HEADER;
        arr[1]=frameLen;
        arr[2]=SerialCMD_WriteMem;
        for(int i=0;i<4;i++)//将地址分为四个字节
            arr[i+3]=(addr>>(i*8))&0xFF;
        for(int i=0;i<data.length();i++)//依次写入数据
            arr[i+7]=data.at(i);
        port->write(arr);
    }
}

//发送串口复位指令
void SerialOCD::sendSerialReset()
{
    if(port->isOpen())
    {
        QByteArray arr;
        arr.resize(3);
        arr[0]=DEBUG_FRAME_HEADER;
        arr[1]=3;
        arr[2]=SerialCMD_Reset;
        port->write(arr);
    }
}
