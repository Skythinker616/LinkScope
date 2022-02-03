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

    gdb=new GDBProcess();//创建并启动GDB
    gdb->setTempSymbolFileName("tmp_list");//设定临时符号文件名
    gdb->start();
}

ListWindow::~ListWindow()
{
    gdb->unloadSymbolFile();//删除临时文件
    gdb->stop();//结束GDB进程
    delete gdb;
    delete treeModel;
    delete ui;
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
        QString rawVarList=gdb->runCmd("info variables\r\n");//使用info variables指令列出axf中所有变量
        gdb->removeInnerSection(rawVarList,0);//移除内部直接嵌套的部分
        QStringList varList=gdb->getVarListFromRawOutput(rawVarList);//解析出变量列表
        foreach(QString name,varList)//依次添加子节点并更新可展开状态
        {
            node.append(name);
            node.children.last().expandable=gdb->checkExpandableType(getVarFullName(node.children.last()));
        }
    }
    else//传入的是普通节点
    {
        QString fullName=getVarFullName(node);//计算节点全名
        QString rawVarType=gdb->runCmd(QString("whatis %1\r\n").arg(fullName));//使用whatis指令获取类型
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
                    node.children[0].expandable=gdb->checkExpandableType(getVarFullName(node.children[0]));
                else//若不是第一个元素，直接用前一个元素的可展开状态即可
                    node.children[i].expandable=node.children[i-1].expandable;
            }
        }
        else if(!rawVarType.contains('('))//不是数组类型，排除函数类型，判定是否为可展开类型
        {
            QString detailRawVarType=gdb->runCmd(QString("ptype %1\r\n").arg(fullName));//用ptype指令获取详细类型
            detailRawVarType.remove("type = ");
            detailRawVarType.remove("\r\n(gdb) ");
            if(detailRawVarType.startsWith("struct")||detailRawVarType.startsWith("union")||detailRawVarType.startsWith("class"))//判定为可展开类型
            {
                gdb->removeInnerSection(detailRawVarType,detailRawVarType.indexOf('{')+1);//移除内部直接嵌套的部分
                QStringList varList=gdb->getVarListFromRawOutput(detailRawVarType);//解析出变量列表
                foreach(QString name,varList)//依次添加子节点及其可展开状态
                {
                    node.append(name);
                    node.children.last().expandable=gdb->checkExpandableType(getVarFullName(node.children.last()));
                }
            }
        }
    }
    node.parsed=true;//更新该节点为已解析状态
}

//加载AXF文件
void ListWindow::loadNewAxfFile(const QString &path)
{
    gdb->loadSymbolFile(path);
    updateTree();//更新树状图
}

//更新整个树状图和节点树
void ListWindow::updateTree()
{
    varTree.clear();//从根节点开始递归清理
    varTree.expandable=true;//设定根节点可展开

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
            if(childNode.expandable)//若该子节点可展开，则添加一个空item使其显示为可展开状态
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
