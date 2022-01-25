#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("LinkScope");

    setStylesheet();//设置全局样式表

    graph=new GraphWindow();//创建并显示绘图窗口
    graph->setVarList(&varList);
    graph->show();

    stampTimer=new QElapsedTimer();//创建并运行时间戳定时器
    stampTimer->start();

    watchTimer=new QTimer(this);//创建watch定时器
    watchTimer->setInterval(10);
    watchTimer->stop();
    connect(watchTimer,SIGNAL(timeout()),this,SLOT(slotWatchTimerTrig()));

    tableTimer=new QTimer(this);//创建表格刷新定时器
    tableTimer->setInterval(200);
    tableTimer->stop();
    connect(tableTimer,SIGNAL(timeout()),this,SLOT(slotTableTimerTrig()));

    tableModel=new QStandardItemModel(this);//创建并初始化表格
    initTable();

    ocdProcess=new QProcess(0);//创建openocd进程
    connect(ocdProcess,SIGNAL(readyReadStandardError()),this,SLOT(slotOCDErrorReady()));

    gdbProcess=new QProcess(0);//创建并运行gdb进程
    setGDBState(true);

    loadConfFileList();//从openocd文件夹中读取配置文件列表
}

MainWindow::~MainWindow()
{
    if(connected)//若处于连接状态则断开连接
        setConnState(false);
    setGDBState(false);//结束gdb进程
    delete ui;
    delete tableModel;
    delete ocdProcess;
    delete gdbProcess;
    delete stampTimer;
    delete watchTimer;
    delete tableTimer;
    delete graph;
}

//按键事件，监听DEL键按下，用于删除单个变量
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Delete)//判定是DEL键
    {
        if(ui->tb_var->currentIndex().column()==0)//确定已选中某个变量的变量名
        {
            int index=ui->tb_var->currentIndex().row();//获取所选变量的下标
            if(index<varList.size())
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle("提示");
                msgBox.setText("确认要删除变量 "+varList.at(index).name+" 吗？");
                msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
                if(msgBox.exec()==QMessageBox::Yes)//用户确认后移除变量并重绘表格
                {
                    varList.removeAt(index);
                    redrawTable();
                }
            }
        }
    }
}

//窗口关闭事件，在主窗口关闭时自动关闭绘图窗口以退出程序
void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    graph->close();
}

//ocd进程发生错误，自动断开连接
void MainWindow::slotOCDErrorReady()
{
    QString error=ocdProcess->readAllStandardError();
    if(error.contains("Error:"))//错误信息中含有Error项，则直接断开连接并弹出提示
    {
        setConnState(false);
        QMessageBox::information(this,"连接错误",error);
    }
}

//表格被编辑，添加变量或修改变量值
void MainWindow::slotTableEdit(QModelIndex topleft, QModelIndex bottomright)
{
    Q_UNUSED(bottomright);
    if(topleft.column()==0 && topleft.row()==varList.size())//若编辑的是最后一行的变量名，进行变量添加
    {
        QString name=tableModel->item(topleft.row(),0)->text();
        if(!name.isEmpty())
        {
            VarInfo var;//创建一个默认配置的变量并添加到列表
            var.name=name;
            var.rawValue="";
            var.enableScope=true;
            var.lineColor=QColor(Qt::black);
            varList.append(var);

            redrawTable();//重绘表格

            if(connected)//若正在连接状态则向gdb发送新的变量列表
                setGDBDispList();
        }
    }
    else if(topleft.column()==2 && topleft.row()!=varList.size())//若编辑的是第二列，表示需进行变量值修改
    {
        QString name=varList.at(topleft.row()).name;
        QString valueStr=tableModel->item(topleft.row(),2)->text();
        if(!valueStr.isEmpty())
        {
            setVar(name,valueStr.toDouble());//向gdb发送命令写入变量值
            tableModel->item(topleft.row(),2)->setText("");//清空编辑框
        }
    }
}

//watch定时器触发，进行一次变量查看
void MainWindow::slotWatchTimerTrig()
{
    if(isWatchProcessing)//若上次查看还未完成则直接退出
        return;
    isWatchProcessing=true;

    QString rawGDB;
    getGDBRawDisp(rawGDB);//获取gdb查看得到的原始值
    parseGDBRawDisp(rawGDB);//进行正则匹配，更新变量列表内容

    isWatchProcessing=false;
}

//表格刷新定时器触发，刷新各变量当前值
void MainWindow::slotTableTimerTrig()
{
    for(int index=0;index<varList.size();index++)//遍历每个变量
    {
        QStandardItem *item=tableModel->item(index,1);
        if(item->text()!=varList[index].rawValue)//若列表中数据与当前显示的不同，表示变量发生变化
        {
            item->setText(varList[index].rawValue);//设置新值
            item->setBackground(QBrush(QColor("#99CCFF")));//设置单元格背景为淡蓝色
        }
        else
        {
            item->setBackground(QBrush(QColor(Qt::white)));//若不发生变化则设置单元格背景为白色
        }
    }
}

//连接按钮点击，触发连接状态切换
void MainWindow::on_bt_conn_clicked()
{
    if(!connected && !axfChosen)//用户未选择axf文件就点击连接按钮
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("警告");
        msgBox.setText("还未选择符号文件，连接后仅能使用绝对地址查看变量，是否继续连接？");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec()!=QMessageBox::Ok)
            return;
    }
    setConnState(!connected);//切换连接状态
}

//设置连接状态（参数true表示进行连接，false表示断开连接）
void MainWindow::setConnState(bool connect)
{
    if(connect)//进行连接
    {
        ui->bt_conn->setEnabled(false);//先禁用连接按钮，防止多次点击
        setOCDState(true);//运行openocd进程进行目标连接
        sleep(500);//等待500ms
        if(ocdProcess->state()==QProcess::Running)//若ocd成功启动
        {
            setGDBConnState(true);//连接gdb到ocd
            for(int i=0;i<varList.size();i++)//清空变量列表历史采样点数据
                varList[i].samples.clear();
            stampTimer->restart();//开启时间戳计时
            watchTimer->start();//开启定时查看变量值
            tableTimer->start();//开启表格定时刷新
            ui->bt_conn->setText("断开连接");
            ui->bt_reset->setEnabled(true);//使能复位按钮
            connected=true;//更新连接标志
        }
        ui->bt_conn->setEnabled(true);//恢复连接按钮
    }
    else//断开连接
    {
        watchTimer->stop();//停止定时查看和定时刷新表格
        tableTimer->stop();
        setGDBConnState(false);//断开gdb，停止ocd
        setOCDState(false);
        ui->bt_conn->setText("连接目标");
        ui->bt_reset->setEnabled(false);//禁用复位按钮
        connected=false;//更新连接标志
    }
}

//参数true：运行ocd进程并连接目标；参数false：结束ocd进程
void MainWindow::setOCDState(bool connect)
{
    if(connect)
    {
        ocdProcess->setWorkingDirectory(QCoreApplication::applicationDirPath()+"/openocd/bin");//设置工作路径
        ocdProcess->setProgram(QCoreApplication::applicationDirPath()+"/openocd/bin/openocd.exe");//设置程序路径
        ocdProcess->setNativeArguments(
                    QString("-f interface/%1 -f target/%2")
                    .arg(ui->cb_interface->currentText())
                    .arg(ui->cb_target->currentText()));//设置参数为所选的调试器和目标芯片
        ocdProcess->start();
    }
    else
    {
        if(ocdProcess->state()==QProcess::NotRunning)
            return;
        QProcess killProcess(0);//创建新进程，用taskkill强行结束ocd进程
        killProcess.setProgram("taskkill");
        killProcess.setNativeArguments(QString("/F /PID %1").arg(ocdProcess->pid()->dwProcessId));
        killProcess.start();
        killProcess.waitForFinished();
    }
}

//参数true：运行gdb进程；参数false：结束gdb进程
void MainWindow::setGDBState(bool run)
{
    if(run)
    {
        gdbProcess->setProgram(QCoreApplication::applicationDirPath()+"/gdb/gdb.exe");//设置程序路径
        gdbProcess->setWorkingDirectory(QCoreApplication::applicationDirPath()+"/gdb");//设置工作路径
        gdbProcess->setNativeArguments("-q");//设置gdb在安静模式下打开
        gdbProcess->start();
    }
    else
    {
        gdbProcess->kill();
    }
}

//参数true：gdb连接到本机3333端口，设置调试参数；参数false：断开gdb连接
void MainWindow::setGDBConnState(bool connect)
{
    QString tmpFilePath=QCoreApplication::applicationDirPath()+"/gdb/tmp";//临时符号文件路径
    if(connect)
    {
        gdbProcess->write("target remote localhost:3333\r\n");//连接到3333端口
        QFile::remove(tmpFilePath);//确保删除当前的临时文件
        QFile::copy(ui->txt_axf_path->text(),tmpFilePath);//将所选符号文件复制为临时文件
        gdbProcess->write(QString("symbol-file %1 \r\n").arg("tmp").toStdString().c_str());//设置符号文件
        gdbProcess->write("set confirm off\r\n");//设置不要手动确认
        gdbProcess->write("set print pretty on\r\n");//设置结构体规范打印
        setGDBDispList();//向gdb发送当前的变量列表
    }
    else
    {
        gdbProcess->write("symbol-file\r\n");//取消符号文件
        gdbProcess->write("disconnect\r\n");//断开gdb连接
        sleep(10);//确保对临时文件取消占用
        QFile::remove(tmpFilePath);//删除复制过来的临时文件
    }
}

//向gdb发送变量列表
void MainWindow::setGDBDispList()
{
    gdbProcess->write("delete display\r\n");//删除之前发送的变量列表
    foreach(VarInfo info,varList)
        gdbProcess->write(QString("display %1 \r\n").arg(info.name).toStdString().c_str());//向display表中依次添加变量名
}

//向gdb请求读取所有变量，获取返回的原始字符串
void MainWindow::getGDBRawDisp(QString &raw)
{
    if(!connected)
        return;
    gdbProcess->readAllStandardOutput();
    gdbProcess->write("display\r\n");//向gdb发送display指令
    raw="";
    do{
        gdbProcess->waitForReadyRead();
        raw+=gdbProcess->readAllStandardOutput();
    }while(!raw.endsWith("(gdb) "));//不断读取直到读到字符串"gdb "，表示指令执行完成
}

//解析由gdb返回的display原始字符串，更新变量值
void MainWindow::parseGDBRawDisp(QString &raw)
{
    qint64 timestamp=stampTimer->nsecsElapsed()/1000;//获取时间戳

    for(int index=0;index<varList.size();index++)//依次进行每个变量的匹配
    {
        VarInfo &info=varList[index];

        QString regName="";
        for(int i=0;i<info.name.length();i++)//将变量名每个转换为16进制格式，用于正则匹配
            regName+=QString("\\x%1").arg(info.name.at(i).unicode(),0,16);

        QRegExp rx(QString("\\d+:\\s%1\\s=\\s(.*)\\r\\n[\\d\\(]").arg(regName));//正则匹配模板，匹配选中的变量名并截取出变量值
        rx.setMinimal(true);//使用非贪心模式

        if(rx.indexIn(raw)!=-1)
        {
            info.rawValue=rx.cap(1);//正则中截取出的字符串即为变量当前值
            SamplePoint sample;
            if(getValueFromRaw(info.rawValue,sample.value))//尝试转换为double，生成采样点并写入采样点列表
            {
                sample.timestamp=timestamp;
                info.samples.append(sample);
            }
        }
    }
}

//延时制定毫秒数，进行子任务循环
void MainWindow::sleep(uint32_t ms)
{
    QTime untilTime=QTime::currentTime().addMSecs(ms);
    while(QTime::currentTime()<untilTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
}

//读取ocd目录下的配置文件列表，添加到下拉框
void MainWindow::loadConfFileList()
{
    ui->cb_interface->clear();
    ui->cb_target->clear();
    QDir interfaceDir(QCoreApplication::applicationDirPath()+"/openocd/share/openocd/scripts/interface");
    interfaceDir.setFilter(QDir::Files);
    foreach(QFileInfo fileInfo,interfaceDir.entryInfoList())
        ui->cb_interface->addItem(fileInfo.fileName());
    QDir targetDir(QCoreApplication::applicationDirPath()+"/openocd/share/openocd/scripts/target");
    targetDir.setFilter(QDir::Files);
    foreach(QFileInfo fileInfo,targetDir.entryInfoList())
        ui->cb_target->addItem(fileInfo.fileName());
}

//设置axf文件按钮点击，弹出文件选框，读取用户选择的路径
void MainWindow::on_bt_set_axf_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this);//弹出文件选择框
    fileDialog->setWindowTitle(QStringLiteral("选中文件"));
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("AXF/ELF File (*.axf *.elf)"));//设置文件过滤器为axf/elf
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setViewMode(QFileDialog::Detail);
    if(fileDialog->exec())
    {
        QStringList fileList=fileDialog->selectedFiles();
        QString fileName=fileList.at(0);
        QFileInfo info(fileName);
        ui->txt_axf_path->setText(info.filePath());
        axfChosen=true;
    }
}

//初始化表格
void MainWindow::initTable()
{
    redrawTable();
    connect(tableModel,SIGNAL(dataChanged(QModelIndex,QModelIndex)),this,SLOT(slotTableEdit(QModelIndex,QModelIndex)));
    ui->tb_var->setModel(tableModel);
}

//重绘表格，设置表头并添加各变量信息
void MainWindow::redrawTable()
{
    tableModel->clear();
    tableModel->setColumnCount(5);//设置表格为5列
    tableModel->setHeaderData(0,Qt::Horizontal,"变量名");//设置表头
    tableModel->setHeaderData(1,Qt::Horizontal,"当前值");
    tableModel->setHeaderData(2,Qt::Horizontal,"修改变量");
    tableModel->setHeaderData(3,Qt::Horizontal,"使能绘图");
    tableModel->setHeaderData(4,Qt::Horizontal,"图线颜色");

    for(int i=0;i<varList.size();i++)//依次添加各变量信息
    {
        tableModel->setItem(i,0,new QStandardItem(varList.at(i).name));
        tableModel->setItem(i,1,new QStandardItem(varList.at(i).rawValue));
        tableModel->setItem(i,2,new QStandardItem(""));
        QStandardItem *checkItem=new QStandardItem();
        checkItem->setCheckable(true);
        checkItem->setCheckState(varList.at(i).enableScope?Qt::Checked:Qt::Unchecked);
        tableModel->setItem(i,3,checkItem);
        tableModel->setItem(i,4,new QStandardItem(varList.at(i).lineColor.name()));
        tableModel->item(i,4)->setForeground(QBrush(varList.at(i).lineColor));
        tableModel->item(i,0)->setFlags(Qt::ItemIsEnabled);
        tableModel->item(i,1)->setFlags(Qt::ItemIsEnabled);
        tableModel->item(i,4)->setFlags(Qt::ItemIsEnabled);
    }
    int lastRow=varList.size();
    tableModel->setItem(lastRow,0,new QStandardItem(""));//末尾添加空行，用于用户添加变量
}

//发送gdb指令，修改变量值
void MainWindow::setVar(const QString &name, double value)
{
    if(!connected)
        return;
    gdbProcess->write(QString("set %1=%2\r\n").arg(name).arg(value).toStdString().c_str());
}

//尝试从变量值原始字符串中解析出double值，返回是否解析成功
bool MainWindow::getValueFromRaw(const QString &rawValue,double &value)
{
    if(rawValue.isEmpty())
        return false;
    if(rawValue.contains('{')||rawValue.contains('(')
            ||rawValue.contains('<')||rawValue.contains('['))//若含有这些符号，表示该变量可能为复合类型，不能被解析
        return false;
    value=rawValue.mid(0,rawValue.indexOf(' ')).toDouble();
    return true;
}

//保存配置到指定路径的文件
void MainWindow::saveToFile(const QString &filename)
{
    QSettings settings(filename,QSettings::IniFormat);
    settings.setIniCodec("GBK");

    settings.beginGroup("Global");//写入全局配置
    settings.setValue("Interface",ui->cb_interface->currentText());
    settings.setValue("Target",ui->cb_target->currentText());
    settings.setValue("AxfChosen",axfChosen);
    settings.setValue("AxfPath",ui->txt_axf_path->text());
    settings.setValue("VarNum",varList.size());
    settings.endGroup();

    for(int index=0;index<varList.size();index++)//写入每个变量的配置信息（不会写入采样点数据）
    {
        const VarInfo *varInfo=&varList.at(index);
        settings.beginGroup(QString("VarInfo%1").arg(index));
        settings.setValue("Name",varInfo->name);
        settings.setValue("EnableScope",varInfo->enableScope);
        settings.setValue("LineColor",varInfo->lineColor.name());
        settings.endGroup();
    }
}

//从指定路径的文件中读取配置
void MainWindow::loadFromFile(const QString &filename)
{
    QSettings settings(filename,QSettings::IniFormat);
    settings.setIniCodec("GBK");

    settings.beginGroup("Global");//读取全局配置
    ui->cb_interface->setCurrentText(settings.value("Interface").toString());
    ui->cb_target->setCurrentText(settings.value("Target").toString());
    axfChosen=settings.value("AxfChosen",true).toBool();
    ui->txt_axf_path->setText(settings.value("AxfPath").toString());
    int varNum=settings.value("VarNum").toInt();
    settings.endGroup();

    varList.clear();
    for(int index=0;index<varNum;index++)//依次读取各变量配置
    {
        VarInfo varInfo;
        settings.beginGroup(QString("VarInfo%1").arg(index));
        varInfo.name=settings.value("Name").toString();
        varInfo.enableScope=settings.value("EnableScope").toBool();
        varInfo.lineColor=QColor(settings.value("LineColor").toString());
        settings.endGroup();
        varList.append(varInfo);
    }

    redrawTable();//重绘表格
}

//导出变量采样数据到指定CSV文件中
bool MainWindow::exportCSV(const QString &filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return false;
    QTextStream outStream(&file);//使用流方式输出

    int maxSampleNum=0;
    for(int i=0;i<varList.size();i++)//计算最大采样点数
        if(varList.at(i).samples.size()>maxSampleNum)
            maxSampleNum=varList.at(i).samples.size();

    for(int index=0;index<varList.size();index++)//写入表头
        outStream<<"Timestamp,"<<varList.at(index).name<<',';
    outStream<<'\n';

    for(int row=0;row<maxSampleNum;row++)//依次写入各变量的采样数据
    {
        for(int index=0;index<varList.size();index++)
        {
            const VarInfo &varInfo=varList.at(index);
            if(row<varInfo.samples.size())
                outStream<<varInfo.samples.at(row).timestamp<<','<<varInfo.samples.at(row).value<<',';
            else
                outStream<<",,";
        }
        outStream<<'\n';
    }

    return true;
}

//复位按钮点击，向gdb发送复位指令
void MainWindow::on_bt_reset_clicked()
{
    if(connected)
        gdbProcess->write("monitor reset\r\n");
}

//表格双击事件，进行图线颜色修改
void MainWindow::on_tb_var_doubleClicked(const QModelIndex &index)
{
    if(index.column()==4&&index.row()<varList.size())//点击的是第4列，说明要修改颜色
    {
        QStandardItem *item=tableModel->item(index.row(),index.column());
        QColorDialog colorDialog(this);//弹出颜色选择框，请求用户选择颜色
        colorDialog.setWindowTitle("请选择颜色");
        colorDialog.setCurrentColor(QColor(item->text()));
        if(colorDialog.exec()==QColorDialog::Accepted)//用户确定了颜色选择
        {
            QColor color=colorDialog.selectedColor();//获取用户选择的颜色
            varList[index.row()].lineColor=color;
            item->setForeground(QBrush(color));
            item->setText(color.name());
        }
    }
}

//表格单击事件
void MainWindow::on_tb_var_clicked(const QModelIndex &index)
{
    if(index.column()==3&&index.row()<varList.size())//在第3列上，表示用户切换了绘图使能状态
    {
        varList[index.row()].enableScope=(tableModel->item(index.row(),index.column())->checkState()==Qt::Checked);
    }

    if(index.column()==0&&index.row()<varList.size())//在变量名列，表示用户选中了该变量
    {
        graph->setChosenIndex(index.row());//设置绘图界面关注该变量
    }
    else //若点击其他列则取消选择
    {
        graph->setChosenIndex(-1);
    }
}

//保存配置菜单点击
void MainWindow::on_action_save_triggered()
{
    //弹出文件选择框
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(QStringLiteral("选中文件"));
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("Ini File (*.ini)"));
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setViewMode(QFileDialog::Detail);
    if(fileDialog->exec())//等待用户选择文件
    {
        QStringList fileList=fileDialog->selectedFiles();
        QString fileName=fileList.at(0);
        if(!fileName.endsWith(".ini"))//若用户输入的文件名不以ini结尾则加上后缀
            fileName.append(".ini");
        QFileInfo file(fileName);//判断是否已经存在文件，若存在则弹出覆盖警告框
        if(file.exists())
        {
            QMessageBox messageBox(QMessageBox::NoIcon,"警告", "文件已存在，是否覆盖？",QMessageBox::Yes | QMessageBox::No, NULL);
            if(messageBox.exec()!=QMessageBox::Yes)
                return;
        }
        saveToFile(fileName);//将配置写入文件
    }
}

//导入配置菜单点击
void MainWindow::on_action_load_triggered()
{
    //弹出文件选择框
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(QStringLiteral("选中文件"));
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("Ini File (*.ini)"));
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setViewMode(QFileDialog::Detail);
    if(fileDialog->exec())//等待用户选择文件
    {
        QStringList fileList=fileDialog->selectedFiles();
        QString fileName=fileList.at(0);
        loadFromFile(fileName);//从文件中读取配置
    }
}

//导出数据菜单点击
void MainWindow::on_action_export_triggered()
{
    //弹出文件选择框
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(QStringLiteral("选中文件"));
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("CSV File (*.csv)"));
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setViewMode(QFileDialog::Detail);
    if(fileDialog->exec())//等待用户选择文件
    {
        QStringList fileList=fileDialog->selectedFiles();
        QString fileName=fileList.at(0);
        if(!fileName.endsWith(".csv"))//若用户输入的文件名不以csv结尾则加上后缀
            fileName.append(".csv");
        QFileInfo file(fileName);//判断是否已经存在文件，若存在则弹出覆盖警告框
        if(file.exists())
        {
            QMessageBox messageBox(QMessageBox::NoIcon,"警告", "文件已存在，是否覆盖？",QMessageBox::Yes | QMessageBox::No, NULL);
            if(messageBox.exec()!=QMessageBox::Yes)
                return;
        }
        if(!exportCSV(fileName))//将数据写入文件
        {
            QMessageBox::information(this,"错误","文件打开失败，请检查文件是否被占用");
        }
    }
}

//关于菜单栏点击
void MainWindow::on_action_about_triggered()
{
    //弹出messagebox显示关于信息
    QString str="LinkScope 版本号：V1.0.1\n\n"
                "Developed by Skythinker";
    QMessageBox box;
    box.setWindowTitle("关于 LinkScope");
    box.setText(str);
    box.exec();
}

//帮助菜单栏点击
void MainWindow::on_action_help_triggered()
{
    //弹出messagebox显示帮助信息
    QString str="LinkScope简介\n"
                "本程序使用QT编写，基于OpenOCD和GDB，用于硬件设备的调试，可以实时查看并修改变量值，有波形绘制和数据导出功能\n"
                "程序支持OpenOCD支持的各种调试器及硬件芯片，如STLink、JLink、CMSIS-DAP等以及STM32全系列等\n\n"
                "使用方法\n"
                "1.在下拉框中选择调试器和芯片类型，选择Axf文件路径，点击连接即可尝试连接芯片\n"
                "2.在表格最后一行变量名处填写变量名可以添加查看变量，选中变量名按Del键可以删除变量\n"
                "3.编辑【修改变量】列可以修改变量值，双击【图线颜色】列可以选择绘图颜色\n"
                "4.单击【变量名】列选中对应的变量，可以在绘图窗口查看历史数据，并会加粗绘制\n"
                "5.绘图界面说明请到绘图窗口点击操作说明\n"
                "6.点击菜单中的保存/导入配置可以将当前配置保存到INI文件或从文件中恢复配置，点击导出数据可以将获取到的采样数据导出到CSV表格文件\n\n"
                "注意事项\n"
                "1.修改Axf路径后需要重新连接\n"
                "2.在【变量名】列不仅能填写单个变量名，还可以填入任何合法的C语言表达式\n"
                "3.连接目标前请确认已使用该调试器为目标芯片下载过指定程序\n"
                "4.若程序闪退后发现下一次运行时无法连接目标，请尝试手动结束openocd.exe进程\n"
                "5.连接配置文件位于openocd/share/openocd/scripts下的target和interface中，用户可按照openocd语法编写配置脚本，放入对应目录下后点击“刷新连接配置”菜单项\n";
    QMessageBox box;
    box.setWindowTitle("LinkScope 帮助");
    box.setText(str);
    box.exec();
}

//显示绘图窗口菜单点击
void MainWindow::on_action_show_graph_triggered()
{
    graph->show();
    graph->activateWindow();
}

//刷新连接配置菜单点击
void MainWindow::on_action_refresh_conf_triggered()
{
    loadConfFileList();
}

//转到主页菜单点击
void MainWindow::on_action_homepage_triggered()
{
    QDesktopServices::openUrl(QUrl("https://gitee.com/skythinker/link-scope"));//打开仓库主页
}

//设置QSS全局样式
void MainWindow::setStylesheet()
{
    QFile qss(":/qss/light-blue.qss");
    if(qss.open(QIODevice::ReadOnly))
        qApp->setStyleSheet(QLatin1String(qss.readAll()));
}
