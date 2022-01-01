#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include <vartype.h>
#include <qtimer.h>
#include <QKeyEvent>
#include <QPainter>
#include <qdebug.h>
#include <qmessagebox.h>

namespace Ui {
class GraphWindow;
}

class GraphWindow : public QDialog
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    ~GraphWindow();
    void setVarList(QList<VarInfo> *list);
    void setChosenIndex(int varIndex);
    void paintGraph(QWidget *canvas);
    bool eventFilter(QObject *watched, QEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private slots:
    void onTrig();
    void on_btn_help_clicked();

private:
    static const int VERT_DIV=6; //纵向划分的格数
    static const int HORI_DIV=10; //横向划分的格数
    Ui::GraphWindow *ui;
    QList<VarInfo> *varList; //变量列表指针，需外部设置
    QTimer *trigTimer; //定时器用于触发图像更新
    bool ctrlFlag=false,shiftFlag=false,altFlag=false; //组合键按下标志
    bool looking=false; //当前是否正在查看变量
    bool dragging=false; //当前是否正在拖动画布
    QPoint mousePos,lastMousePos; //拖拽时的鼠标坐标
    int chosenVarIndex=-1; //当前选中的变量在列表中的索引（未选中时-1）
    void updateCursor();
};

#endif // GRAPHWINDOW_H
