#include "logwindow.h"
#include "ui_logwindow.h"

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);
    setWindowTitle("LinkScope - Log");

    tableModel=new QStandardItemModel(this);//创建表格数据
    ui->tb_log->setModel(tableModel);//绑定表格数据
    ui->tb_log->horizontalHeader()->setMinimumSectionSize(150);//设置最小列宽
    ui->tb_log->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);//列宽可由用户调整
    ui->tb_log->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);//行高自动拉伸
    ui->tb_log->verticalHeader()->hide();//隐藏垂直表头，否则滚动语句执行时间较长
    drawTableHeader();//设置表头
}

LogWindow::~LogWindow()
{
    delete ui;
    delete tableModel;
}

//添加一条日志
void LogWindow::addLog(char attr, const QString &tag, const QString &msg, qulonglong time, const QString &func)
{
    //计算日志类型字符对应的颜色和属性名
    QColor textColor=Qt::black;
    QString attrText="Unknown";
    switch(attr)
    {
        case 'i':
        textColor=QColor("#218B47");
        attrText="Info";
        break;
        case 'd':
        textColor=QColor("#0046EB");
        attrText="Debug";
        break;
        case 'w':
        textColor=QColor("#FF8C00");
        attrText="Warn";
        break;
        case 'e':
        textColor=QColor("#EB005F");
        attrText="Error";
        break;
    }
    //创建各单元格数据
    QList<QStandardItem*> items;
    items<<new QStandardItem(attrText)
        <<new QStandardItem(tag)
        <<new QStandardItem(msg)
        <<new QStandardItem(QString("%1:%2:%3.%4")
            .arg(time/1000/3600,2,10,QChar('0'))
            .arg(time/1000/60%60,2,10,QChar('0'))
            .arg(time/1000%60,2,10,QChar('0'))
            .arg(time%1000,3,10,QChar('0')))
        <<new QStandardItem(func);
    for(int i=0;i<5;i++)
    {
        items[i]->setForeground(QBrush(textColor));//设置文字颜色
        items[i]->setFlags(Qt::ItemIsEnabled);//设置为不可编辑
    }
    //若当前表格滚动到底端则在插入新行后自动滚动到底端
    bool scrollAtEnd=ui->tb_log->verticalScrollBar()->value()==ui->tb_log->verticalScrollBar()->maximum();
    tableModel->insertRow(tableModel->rowCount(),items);//在表格末尾插入新行
    if(scrollAtEnd)
        ui->tb_log->scrollToBottom();
}

//清空所有日志
void LogWindow::clearLog()
{
    QByteArray horiHeaderState=ui->tb_log->horizontalHeader()->saveState();//记录列宽
    tableModel->clear();
    drawTableHeader();
    ui->tb_log->horizontalHeader()->restoreState(horiHeaderState);//恢复列宽
}

//绘制表头
void LogWindow::drawTableHeader()
{
    tableModel->setColumnCount(5);
    tableModel->setHeaderData(0,Qt::Horizontal,"类型");
    tableModel->setHeaderData(1,Qt::Horizontal,"TAG");
    tableModel->setHeaderData(2,Qt::Horizontal,"MSG");
    tableModel->setHeaderData(3,Qt::Horizontal,"时间戳");
    tableModel->setHeaderData(4,Qt::Horizontal,"函数名");
}

//将表格数据导出到CSV文件
bool LogWindow::exportCSV(const QString &filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return false;
    QTextStream outStream(&file);//使用流方式输出

    outStream<<"Type,TAG,MSG,Timestamp,Function\n";

    for(int row=0;row<tableModel->rowCount();row++)
    {
        for(int col=0;col<tableModel->columnCount();col++)
            outStream<<tableModel->item(row,col)->text()<<',';
        outStream<<'\n';
    }

    return true;
}

//清空日志按钮点击
void LogWindow::on_bt_clear_clicked()
{
    clearLog();
}

//导出日志按钮点击
void LogWindow::on_bt_export_clicked()
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
