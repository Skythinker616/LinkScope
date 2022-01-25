#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vartype.h>
#include <qprocess.h>
#include <qdebug.h>
#include <QTime>
#include <QDir>
#include <qfiledialog.h>
#include <windows.h>
#include <qmessagebox.h>
#include <qstandarditemmodel.h>
#include <qtimer.h>
#include <QKeyEvent>
#include <graphwindow.h>
#include <qelapsedtimer.h>
#include <qcolordialog.h>
#include <qsettings.h>
#include <qregexp.h>
#include <qdesktopservices.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event);

private slots:
    void slotOCDErrorReady();
    void slotTableEdit(QModelIndex topleft, QModelIndex bottomright);
    void slotWatchTimerTrig();
    void slotTableTimerTrig();
    void on_bt_conn_clicked();
    void on_bt_set_axf_clicked();
    void on_bt_reset_clicked();
    void on_tb_var_doubleClicked(const QModelIndex &index);
    void on_tb_var_clicked(const QModelIndex &index);
    void on_action_save_triggered();
    void on_action_load_triggered();
    void on_action_export_triggered();
    void on_action_about_triggered();
    void on_action_help_triggered();
    void on_action_show_graph_triggered();
    void on_action_refresh_conf_triggered();
    void on_action_homepage_triggered();

private:
    Ui::MainWindow *ui;
    QProcess *ocdProcess,*gdbProcess;//ocd和gdb进程指针
    bool connected=false;//标记当前是否已连接
    QStandardItemModel *tableModel;//表格数据
    QList<VarInfo> varList;//变量列表
    QTimer *watchTimer,*tableTimer;//定时器，用于查看变量值和刷新表格
    QElapsedTimer *stampTimer;//时间戳定时器指针
    GraphWindow *graph;//绘图窗口指针
    bool isWatchProcessing=false;//标记当前是否正在处理变量值查看
    bool axfChosen=false;//是否已经选择了axf文件
    void setStylesheet();
    void setConnState(bool connect);
    void setOCDState(bool connect);
    void setGDBState(bool run);
    void setGDBConnState(bool connect);
    void setGDBDispList();
    void getGDBRawDisp(QString &raw);
    void parseGDBRawDisp(QString &raw);
    void sleep(uint32_t ms);
    void loadConfFileList();
    void initTable();
    void redrawTable();
    void setVar(const QString &name,double value);
    bool getValueFromRaw(const QString &rawValue,double &value);
    void saveToFile(const QString &filename);
    void loadFromFile(const QString &filename);
    bool exportCSV(const QString &filename);
};
#endif // MAINWINDOW_H
