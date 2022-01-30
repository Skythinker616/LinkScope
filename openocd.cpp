#include "openocd.h"

OpenOCD::OpenOCD(QObject *parent) : QObject(parent)
{
    process=new QProcess();
    connect(process,&QProcess::readyReadStandardError,[=]{
        QString error=process->readAllStandardError();
        if(error.contains("Error:"))//错误信息中含有Error项，则发送信号
        {
            emit onErrorOccur(error);
        }
    });
}

OpenOCD::~OpenOCD()
{
    delete process;
}

//启动OCD进程并设定调试器类型、芯片类型、gdb端口号
void OpenOCD::start(const QString &chosenInterface,const QString &chosenTarget,int gdbPort)
{
    process->setWorkingDirectory(QCoreApplication::applicationDirPath()+"/openocd/bin");//设置工作路径
    process->setProgram(QCoreApplication::applicationDirPath()+"/openocd/bin/openocd.exe");//设置程序路径
    process->setNativeArguments(
                QString("-f interface/%1 -f target/%2 -c \"gdb_port %3\"")
                .arg(chosenInterface).arg(chosenTarget).arg(gdbPort));//设置参数为所选的调试器、目标芯片和gdb端口
    process->start();
}

//结束OCD进程
void OpenOCD::stop()
{
    if(process->state()==QProcess::NotRunning)
        return;
    QProcess killProcess(0);//创建新进程，用taskkill强行结束ocd进程
    killProcess.setProgram("taskkill");
    killProcess.setNativeArguments(QString("/F /PID %1").arg(process->pid()->dwProcessId));
    killProcess.start();
    killProcess.waitForFinished();
}

//返回当前OCD进程是否正在运行
bool OpenOCD::isRunning()
{
    return process->state()==QProcess::Running;
}
