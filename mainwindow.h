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
#include <listwindow.h>
#include <helpwindow.h>
#include <aboutwindow.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <gdbprocess.h>
#include <openocd.h>
#include <serialocd.h>
#include <logwindow.h>
#include <configwindow.h>

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
    void slotTableEdit(QModelIndex topleft, QModelIndex bottomright);
    void slotWatchTimerTrig();
    void slotTableTimerTrig();
    void slotLogTimerTrig();
    void slotOnVarAdd2Edit(const QString &name);
    void slotOnVarAdd2List(const QString &name);
    void slotOnConnErrorOccur(const QString &info);
    void on_bt_conn_clicked();
    void on_bt_set_axf_clicked();
    void on_bt_reset_clicked();
    void on_tb_var_doubleClicked(const QModelIndex &index);
    void on_tb_var_clicked(const QModelIndex &index);
    void on_tb_var_customContextMenuRequested(const QPoint &pos);
    void on_action_save_triggered();
    void on_action_load_triggered();
    void on_action_export_triggered();
    void on_action_about_triggered();
    void on_action_help_triggered();
    void on_action_refresh_conf_triggered();
    void on_action_homepage_triggered();
    void on_action_feedback_triggered();
    void on_action_checkupdate_triggered();
    void on_action_del_var_triggered();
    void on_rb_openocd_toggled(bool checked);
    void on_rb_serialocd_toggled(bool checked);
    void on_bt_refresh_serial_clicked();
    void on_cb_log_toggled(bool checked);
    void on_cb_ext_openocd_toggled(bool checked);
    void on_action_config_triggered();
    void on_action_del_all_triggered();

private:
    Ui::MainWindow *ui;
    QProcess *ocdProcess;
    GDBProcess *gdb;//GDB进程控制
    OpenOCD *openocd;//OpenOCD进程控制
    SerialOCD *serialocd;//串口ocd控制
    bool connected=false;//标记当前是否已连接
    QStandardItemModel *tableModel;//表格数据
    QList<VarInfo> varList;//变量列表
    QTimer *watchTimer,*tableTimer,*logTimer,*autosaveTimer;//定时器，用于查看变量值、刷新表格、监视日志和定时保存
    QElapsedTimer *stampTimer;//时间戳定时器指针
    GraphWindow *graph;//绘图窗口指针
    bool isWatchProcessing=false;//标记当前是否正在处理变量值查看
    bool axfChosen=false;//是否已经选择了axf文件
    ListWindow *listWindow;//选择窗口指针
    LogWindow *logWindow;//日志窗口指针
    QMenu *tablePopMenu;//右键点击表格时弹出的菜单
    ConfigWindowParam configWindowParam;//配置窗口数据
    void checkUpdate();
    void checkOpenocdProcess();
    void setStylesheet();
    void setConnState(bool connect);
    void sleep(uint32_t ms);
    void loadConfFileList();
    void initTable();
    void redrawTable();
    void updateGDBList();
    void saveToFile(const QString &filename);
    void loadFromFile(const QString &filename);
    bool exportCSV(const QString &filename);
    void loadGlobalConf();
    void saveGlobalConf();
};
#endif // MAINWINDOW_H
