#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QDialog>
#include <qstandarditemmodel.h>
#include <qscrollbar.h>
#include <QTime>
#include <qdebug.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qmessagebox.h>

namespace Ui {
class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();
    void addLog(char attr,const QString &tag,const QString &msg,qulonglong time,const QString &func);
    void clearLog();

private slots:
    void on_bt_clear_clicked();
    void on_bt_export_clicked();

private:
    Ui::LogWindow *ui;
    QStandardItemModel *tableModel;//表格数据
    void drawTableHeader();
    bool exportCSV(const QString &filename);
};

#endif // LOGWINDOW_H
