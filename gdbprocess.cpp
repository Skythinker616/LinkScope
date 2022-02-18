#include "gdbprocess.h"

GDBProcess::GDBProcess(QObject *parent) : QObject(parent)
{
    process=new QProcess();
}

GDBProcess::~GDBProcess()
{
    delete process;
}

//运行一条GDB命令并获取输出字符串
QString GDBProcess::runCmd(const QString &cmd)
{
    process->readAllStandardOutput();
    process->write(cmd.toStdString().c_str());
    QString res="";
    do{
        process->waitForReadyRead(1);
        res+=process->readAllStandardOutput();
    }while(!res.endsWith("(gdb) "));
    return res;
}

//启动GDB进程
void GDBProcess::start()
{
    process->setProgram(QCoreApplication::applicationDirPath()+"/gdb/gdb.exe");//设置程序路径
    process->setWorkingDirectory(QCoreApplication::applicationDirPath()+"/gdb");//设置工作路径
    process->setNativeArguments("-q");//设置gdb在安静模式下打开
    process->start();//QProcess::Unbuffered|QProcess::ReadWrite);
    runCmd("set confirm off\r\n");//设置不要手动确认
    runCmd("set print pretty on\r\n");//设置结构体规范打印
}

//停止GDB进程
void GDBProcess::stop()
{
    if(process->state()!=QProcess::NotRunning)
        process->kill();
}

//命令GDB连接到远程目标，参数为"地址:端口号"
void GDBProcess::connectToRemote(const QString &addr)
{
    runCmd("target remote "+addr+"\r\n");
}

//命令GDB从远处目标断开
void GDBProcess::disconnectFromRemote()
{
    runCmd("disconnect\r\n");
}

//设定临时符号文件名，文件位于GDB程序目录下
void GDBProcess::setTempSymbolFileName(const QString &name)
{
    tempSymbolFileName=name;
}

//加载符号文件，将指定文件复制到临时文件中，并加载到GDB中
void GDBProcess::loadSymbolFile(const QString &path)
{
    unloadSymbolFile();//确保卸载当前的临时文件
    QString tempPath=QCoreApplication::applicationDirPath()+"/gdb/"+tempSymbolFileName;//拼接临时文件路径
    QFile::copy(path,tempPath);//将所选符号文件复制为临时文件
    runCmd("symbol-file "+tempSymbolFileName+"\r\n");//设置符号文件
}

//卸载符号文件
void GDBProcess::unloadSymbolFile()
{
    runCmd("symbol-file\r\n");//取消符号文件，解除文件占用
    QString tempPath=QCoreApplication::applicationDirPath()+"/gdb/"+tempSymbolFileName;//拼接临时文件路径
    QFile::remove(tempPath);//删除复制过来的临时文件
}

//设置display列表
void GDBProcess::setDisplayList(QStringList &list)
{
    runCmd("delete display\r\n");//删除之前发送的变量列表
    foreach(QString name,list)//向display表中依次添加变量名
        runCmd("display "+name+"\r\n");
}

//截取display指令所返回字符串中指定变量名的变量值
QString GDBProcess::captureValueFromDisplay(const QString &rawDisplay, const QString &name)
{
    QString regName="";
    for(int i=0;i<name.length();i++)//将变量名每个转换为16进制格式，用于正则匹配
        regName+=QString("\\x%1").arg(name.at(i).unicode(),0,16);

    QRegExp rx(QString("\\d+:\\s%1\\s=\\s(.*)\\r\\n[\\d\\(]").arg(regName));//正则匹配模板，匹配选中的变量名并截取出变量值
    rx.setMinimal(true);//使用非贪心模式

    if(rx.indexIn(rawDisplay)!=-1)
        return rx.cap(1);//正则中截取出的字符串即为变量当前值
    else
        return "";
}

//尝试将display所返回的变量值转为double，返回值为是否转换成功
bool GDBProcess::getDoubleFromDisplayValue(const QString &rawValue, double &result)
{
    if(rawValue.isEmpty())
        return false;
    if(rawValue.contains('{')||rawValue.contains('(')
            ||rawValue.contains('<')||rawValue.contains('['))//若含有这些符号，表示该变量可能为复合类型，不能被解析
        return false;
    result=rawValue.mid(0,rawValue.indexOf(' ')).toDouble();
    return true;
}

//将gdb所返回的无符号数组变量值转换为uint数组
QList<uint> GDBProcess::getUintArrayFromDisplay(const QString &rawDisplay)
{
    QRegExp rx("\\{(.*)\\}");
    rx.indexIn(rawDisplay);
    QString raw=rx.cap(1);
    raw=raw.replace("\r\n  ","");
    QStringList rawList=raw.split(", ");
    QList<uint> numList;
    for(int index=0;index<rawList.size();index++)
    {
        if(rawList[index].contains('<'))//查找并展开重复值
        {
            QRegExp repeatRx("(\\d+)\\s<repeats\\s(\\d+)\\stimes>");
            repeatRx.indexIn(rawList[index]);
            int num=repeatRx.cap(1).toUInt();
            int times=repeatRx.cap(2).toUInt();
            for(int i=0;i<times;i++)
                numList<<num;
        }
        else
        {
            numList<<rawList[index].toUInt();
        }
    }
    return numList;
}

//命令GDB设定变量值
void GDBProcess::setVarValue(const QString &varFullName, double value)
{
    runCmd(QString("set %1=%2\r\n").arg(varFullName).arg(value));
}

//判断指定变量名是否为可展开类型
bool GDBProcess::checkExpandableType(const QString &varFullName)
{
    QString whatis=runCmd(QString("whatis %1\r\n").arg(varFullName));//先使用whatis指令判断数组和函数指针
    whatis.remove("type = ");
    whatis.remove("\r\n(gdb) ");
    if(whatis.contains("["))
        return true;
    if(whatis.contains('('))
        return false;
    QString ptype=runCmd(QString("ptype %1\r\n").arg(varFullName));//再使用ptype指令判断是否是其他可展开类型
    ptype.remove("type = ");
    if(ptype.startsWith("struct")||ptype.startsWith("union")||ptype.startsWith("class"))
        return true;
    return false;
}

//从GDB原始输出（info variables/ptype等指令）中解析变量列表
QStringList GDBProcess::getVarListFromRawOutput(const QString &rawVarList)
{
    QStringList list;
    QRegExp rootRx("\\s([^ (]*\\(\\*|)([^ ):]+)(\\)\\(.*\\)|\\s:\\s\\d+|);");//正则匹配模板
    rootRx.setMinimal(true);//非贪心匹配
    int pos=0;
    while((pos=rootRx.indexIn(rawVarList,pos))!=-1)
    {
        QString name=rootRx.cap(2);//获取截取出的变量名部分
        if(name.contains('*'))//手动剔除'*'
            name.remove('*');
        if(name.contains('['))//手动剔除数组长度部分
            name.remove(QRegExp("\\[\\d+\\]"));
        if(name!="const")//排除解析出来是保留字的情况
            list.append(name);
        pos+=rootRx.matchedLength();
    }
    return list;
}

//在raw字符串（可能由ptype等指令输出）中删除由'{}'嵌套的部分（跳过前offset个字符）
void GDBProcess::removeInnerSection(QString &raw, int offset)
{
    for(int startPos=offset;startPos<raw.length();startPos++)//startPos指针向后移动查找'{'
    {
        if(raw.at(startPos)=='{')//若找到'{'，开始寻找对应的'}'
        {
            int innerLayer=1;//当前endPos所在的嵌套层级
            for(int endPos=startPos+1;endPos<raw.length();endPos++)//endPos指针向后查找对应的'}'
            {
                if(raw.at(endPos)=='{')//每遇到一个'{'则嵌套层级+1
                    innerLayer++;
                else if(raw.at(endPos)=='}')//每遇到一个'}'则嵌套层级-1
                    innerLayer--;

                if(raw.at(endPos)=='}' && innerLayer==0)//若遇到'}'时嵌套层级为0，表示找到了对应的'}'
                {
                    raw.remove(startPos,endPos-startPos+1);//删除从'{'到'}'的子串
                    startPos--;
                    break;
                }
            }
        }
    }
}
