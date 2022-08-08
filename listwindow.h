#ifndef LISTWINDOW_H
#define LISTWINDOW_H

#include <QDialog>
#include <qprocess.h>
#include <qfile.h>
#include <QTime>
#include <qdebug.h>
#include <QStandardItem>
#include <qmessagebox.h>
#include <gdbprocess.h>

namespace Ui {
class ListWindow;
}

//变量信息结点，用于组成变量树
class VarNode{
public:
    QString name;//变量名
    QList<VarNode> children;//子节点
    VarNode *parent=NULL;//父节点
    bool expandable=false;//可展开状态，即该变量类型是否可被展开
    bool parsed=false;//是否已经解析出子节点信息
    VarNode(const QString &_name="",VarNode *_parent=NULL)
    {
        name=_name;
        parent=_parent;
    }
    void append(const QString &name)//指定变量名添加一个子节点
    {
        VarNode node(name,this);//创建一个子节点并将其父节点设为自己
        children.append(node);
    }
    void clear()//清空数据
    {
        children.clear();
        name="";
        expandable=parsed=false;
    }
};

class ListWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ListWindow(QWidget *parent = nullptr);
    ~ListWindow();
    void loadNewAxfFile(const QString &path);

signals:
    void add2Edit(const QString &name);
    void add2List(const QString &name);

private slots:
    void on_tree_expanded(const QModelIndex &index);
    void on_btn_add2edit_clicked();
    void on_btn_add2list_clicked();
    void on_cb_use_path_toggled(bool checked);

private:
    Ui::ListWindow *ui;
    QProcess *gdbProcess;
    GDBProcess *gdb;
    VarNode varTree;
    QStandardItemModel *treeModel;
    QString getVarFullName(const VarNode &node);
    QString getVarFileName(const VarNode &node,bool isFull);
    void parseVarChildren(VarNode &node);
    void updateTree();
};

#endif // LISTWINDOW_H
