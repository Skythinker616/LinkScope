#include "listwindow.h"
#include "ui_listwindow.h"

ListWindow::ListWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ListWindow)
{
    ui->setupUi(this);
    setWindowTitle("LinkScope - Selector");

    treeModel=new QStandardItemModel(ui->tree);
    ui->tree->setModel(treeModel);

    gdbProcess=new QProcess();

    setGDBState(true);//启动GDB进程
}

ListWindow::~ListWindow()
{
    deleteTempFile();//删除临时文件
    setGDBState(false);//结束GDB进程
    delete gdbProcess;
    delete treeModel;
    delete ui;
}

//设置GDB进程状态
void ListWindow::setGDBState(bool run)
{
    if(run)//启动进程
    {
        gdbProcess->setProgram(QCoreApplication::applicationDirPath()+"/gdb/gdb.exe");//设置程序路径
        gdbProcess->setWorkingDirectory(QCoreApplication::applicationDirPath()+"/gdb");//设置工作路径
        gdbProcess->setNativeArguments("-q");//设置gdb在安静模式下打开
        gdbProcess->start();
        runGDBCmd("set confirm off\r\n");//设置不要手动确认
        runGDBCmd("set print pretty on\r\n");//设置结构体规范打印
    }
    else//结束进程
    {
        gdbProcess->kill();
    }
}

//向GDB进程发送命令并获取标准输出
QString ListWindow::runGDBCmd(const QString &input)
{
    gdbProcess->readAllStandardOutput();
    gdbProcess->write(input.toStdString().c_str());
    QString res="";
    do{
        gdbProcess->waitForReadyRead();
        res+=gdbProcess->readAllStandardOutput();
    }while(!res.endsWith("(gdb) "));
    return res;
}

//使用GDB检查变量名是否为可展开类型
bool ListWindow::checkCanExpand(const QString &varFullName)
{
    QString whatis=runGDBCmd(QString("whatis %1\r\n").arg(varFullName));//先使用whatis指令判断数组和函数指针
    whatis.remove("type = ");
    whatis.remove("\r\n(gdb) ");
    if(whatis.contains("["))
        return true;
    if(whatis.contains('('))
        return false;
    QString ptype=runGDBCmd(QString("ptype %1\r\n").arg(varFullName));//再使用ptype指令判断是否是其他可展开类型
    ptype.remove("type = ");
    if(ptype.startsWith("struct")||ptype.startsWith("union"))
        return true;
    return false;
}

//从原始变量列表字符串中解析出变量列表
QStringList ListWindow::parseVarList(const QString &raw)
{
    QStringList list;
    QRegExp rootRx("\\s([^ (]*\\(\\*|)([^ )]+)(\\)\\(.*\\)|\\s:\\s\\d+|);");//正则匹配模板
    rootRx.setMinimal(true);//非贪心匹配
    int pos=0;
    while((pos=rootRx.indexIn(raw,pos))!=-1)
    {
        QString name=rootRx.cap(2);//获取截取出的变量名部分
        if(name.contains('*'))//手动剔除'*'
            name.remove('*');
        if(name.contains('['))//手动剔除数组长度部分
            name.remove(QRegExp("\\[\\d+\\]"));
        list.append(name);
        pos+=rootRx.matchedLength();
    }
    return list;
}

//删除一个原始变量列表字符串中的嵌套部分，跳过前offset个字符
void ListWindow::removeInnerSection(QString &raw,int offset)
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

//计算一个节点的全名（沿parent指针向上回溯）
QString ListWindow::getVarFullName(const VarNode &node)
{
    const VarNode *curNode=&node;
    QString fullName=node.name;
    while(curNode->parent->parent)//到父节点为根节点为止
    {
        if(curNode->name.contains('['))//若为数组元素，父节点为数组名，直接拼接（无需加'.'）
            fullName=curNode->parent->name+fullName;
        else//非数组类型与父节点拼接时还需加一个'.'
            fullName=curNode->parent->name+'.'+fullName;
        curNode=curNode->parent;//指针向上移动一层
    }
    return fullName;
}

//解析一个节点的子节点，并更新子节点的可展开状态
void ListWindow::parseVarChildren(VarNode &node)
{
    if(node.parent==NULL)//传入的是根节点
    {
        QString rawVarList=runGDBCmd("info variables\r\n");//使用info variables指令列出axf中所有变量
        QStringList varList=parseVarList(rawVarList);//解析出变量列表
        foreach(QString name,varList)//依次添加子节点并更新可展开状态
        {
            node.append(name);
            node.children.last().canExpand=checkCanExpand(getVarFullName(node.children.last()));
        }
    }
    else//传入的是普通节点
    {
        QString fullName=getVarFullName(node);//计算节点全名
        QString rawVarType=runGDBCmd(QString("whatis %1\r\n").arg(fullName));//使用whatis指令获取类型
        rawVarType.remove("type = ");
        rawVarType.remove("\r\n(gdb) ");
        if(rawVarType.contains("["))//判定是否为数组类型
        {
            QRegExp rx("\\[(\\d+)\\]");//使用正则表达式提取出第一维长度len
            rx.setMinimal(true);
            rx.indexIn(rawVarType);
            int len=rx.cap(1).toInt();
            for(int i=0;i<len;i++)//添加len个子节点
            {
                node.append(QString("[%1]").arg(i));
                if(i==0)//若是第一个元素，计算是否可展开
                    node.children[0].canExpand=checkCanExpand(getVarFullName(node.children[0]));
                else//若不是第一个元素，直接用前一个元素的可展开状态即可
                    node.children[i].canExpand=node.children[i-1].canExpand;
            }
        }
        else if(!rawVarType.contains('('))//不是数组类型，排除函数类型，判定是否为可展开类型
        {
            QString detailRawVarType=runGDBCmd(QString("ptype %1\r\n").arg(fullName));//用ptype指令获取详细类型
            detailRawVarType.remove("type = ");
            detailRawVarType.remove("\r\n(gdb) ");
            if(detailRawVarType.startsWith("struct")||detailRawVarType.startsWith("union"))//判定为可展开类型
            {
                removeInnerSection(detailRawVarType,detailRawVarType.indexOf('{')+1);//移除内部直接嵌套的部分
                QStringList varList=parseVarList(detailRawVarType);//解析出变量列表
                foreach(QString name,varList)//依次添加子节点及其可展开状态
                {
                    node.append(name);
                    node.children.last().canExpand=checkCanExpand(getVarFullName(node.children.last()));
                }
            }
        }
    }
    node.parsed=true;//更新该节点为已解析状态
}

//加载AXF文件
void ListWindow::loadNewAxfFile(const QString &path)
{
    updateTempFile(path);//更新AXF文件配置
    updateTree();//更新树状图
}

//更新临时文件
void ListWindow::updateTempFile(const QString &path)
{
    QString tmpFilePath=QCoreApplication::applicationDirPath()+"/gdb/tmp_list";//临时符号文件路径
    deleteTempFile();//确保删除当前的临时文件
    QFile::copy(path,tmpFilePath);//将所选符号文件复制为临时文件
    runGDBCmd("symbol-file tmp_list \r\n");//设置符号文件
}

//删除临时文件
void ListWindow::deleteTempFile()
{
    runGDBCmd("symbol-file\r\n");//取消符号文件，解除文件占用
    QFile::remove(QCoreApplication::applicationDirPath()+"/gdb/tmp_list");//删除复制过来的临时文件
}

//更新整个树状图和节点树
void ListWindow::updateTree()
{
    varTree.clear();//从根节点开始递归清理
    varTree.canExpand=true;//设定根节点可展开

    QStandardItem *rootItem=new QStandardItem("");//创建根节点对应的item
    rootItem->setData((qlonglong)&varTree);//将item的数据指向根节点
    rootItem->appendRow(new QStandardItem());//附加一个空item让树状图显示为可展开

    treeModel->clear();//清理树状图数据
    treeModel->appendRow(rootItem);//添加根节点item
}

//树状图展开槽函数
void ListWindow::on_tree_expanded(const QModelIndex &index)
{
    QStandardItem *item=treeModel->itemFromIndex(index);//获取所展开的item
    VarNode &node=*(VarNode*)(item->data().toULongLong());//获取对应的节点
    if(!node.parsed)//若节点未解析过，则进行解析
    {
        parseVarChildren(node);//解析该节点的子节点，更新各子节点的可展开状态
        item->removeRows(0,item->rowCount());//先清空当前item下的所有子项
        for(int i=0;i<node.children.length();i++)//依次创建并添加各子节点的item
        {
            VarNode &childNode=node.children[i];
            QStandardItem *childItem=new QStandardItem();//创建item
            childItem->setText(childNode.name);//设定item显示变量名
            childItem->setData((qlonglong)&childNode);//建立item到对应节点的寻址方式
            if(childNode.canExpand)//若该子节点可展开，则添加一个空item使其显示为可展开状态
                childItem->appendRow(new QStandardItem());
            item->appendRow(childItem);//子节点item附加到当前被展开的item下
        }
    }
}

//添加到编辑框按钮槽函数
void ListWindow::on_btn_add2edit_clicked()
{
    QModelIndex index=ui->tree->currentIndex();//获取当前所选index
    if(index.row()==-1)
        return;
    VarNode &node=*(VarNode*)(treeModel->itemFromIndex(index)->data().toULongLong());//获取所选节点
    if(node.parent)
        emit add2Edit(getVarFullName(node));//将变量全名使用信号发出去
}

//添加到列表按钮槽函数（逻辑与添加到编辑框相同，仅发送的信号不同）
void ListWindow::on_btn_add2list_clicked()
{
    QModelIndex index=ui->tree->currentIndex();
    if(index.row()==-1)
        return;
    VarNode &node=*(VarNode*)(treeModel->itemFromIndex(index)->data().toULongLong());
    if(node.parent)
        emit add2List(getVarFullName(node));
}
