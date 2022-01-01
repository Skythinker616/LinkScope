#include "graphwindow.h"
#include "ui_graphwindow.h"

GraphWindow::GraphWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GraphWindow)
{
    ui->setupUi(this);
    setWindowTitle("LinkScope - Graph");
    ui->txt_graph->installEventFilter(this);
    updateCursor();
    //设定触发定时器
    trigTimer=new QTimer(this);
    trigTimer->setInterval(30); //30ms绘制一帧图像
    trigTimer->start();
    connect(trigTimer,SIGNAL(timeout()),this,SLOT(onTrig()));
}

GraphWindow::~GraphWindow()
{
    delete trigTimer;
    delete ui;
}

//绘图事件，进行图像重绘
void GraphWindow::paintGraph(QWidget *canvas)
{
    //绘图部分在窗口上的位置
    float offsetx=ui->txt_graph->pos().x(),offsety=ui->txt_graph->pos().y();
    //绘图部分宽高
    float hei=ui->txt_graph->height(),wid=ui->txt_graph->width();
    float posx=0,posy=0;//绘图偏移

    QPainter painter(canvas); //画笔

    painter.setBrush(QBrush(QColor(Qt::white)));//填充背景
    painter.drawRect(posx,posy,wid,hei);

    //绘制标记线
    painter.setPen(QPen(QColor(184,184,184),2,Qt::DotLine));
    for(int i=1;i<VERT_DIV;i++)
    {
        float horiY=hei*i/VERT_DIV+posy;
        painter.drawLine(posx,horiY,posx+wid,horiY);
    }
    for(int i=1;i<HORI_DIV;i++)
    {
        float vertX=wid*i/HORI_DIV+posx;
        painter.drawLine(vertX,posy,vertX,posy+hei);
    }

    //计算采样点最大时间范围和需要显示的时间范围
    double totalTime=0;
    for(int varIndex=0;varIndex<varList->size();varIndex++)
        if(varList->at(varIndex).samples.size()>0 && varList->at(varIndex).samples.last().timestamp>totalTime*1000000)
            totalTime=varList->at(varIndex).samples.last().timestamp/1000000.0;
    double timeRange=ui->sb_hori_rsv->value()*HORI_DIV;

    if(dragging)//若正在拖拽则根据鼠标位移移动图像
    {
        double vertDist=(mousePos.y()-lastMousePos.y())*1.0/hei*ui->sb_vert_rsv->value()*VERT_DIV;
        ui->sb_vert_offset->setValue(ui->sb_vert_offset->value()-vertDist);
        double horiDist=(mousePos.x()-lastMousePos.x())*1.0/wid*timeRange*1000;
        ui->bar_hori->setValue(ui->bar_hori->value()-horiDist);
        lastMousePos=mousePos;
    }

    //设定水平拖动条范围
    if(totalTime>timeRange)
        ui->bar_hori->setMaximum(totalTime*1000-timeRange*1000);
    else
        ui->bar_hori->setMaximum(1);
    //若勾选实时更新则设定拖动条到末端
    if(ui->cb_update->checkState()==Qt::Checked)
        ui->bar_hori->setValue(ui->bar_hori->maximum());

    double startTime=ui->bar_hori->value()*1.0/ui->bar_hori->maximum()*(totalTime-timeRange);//需显示部分的开始时间
    double endTime=startTime+timeRange;//需显示部分的结束时间
    double valueRange=ui->sb_vert_rsv->value()*VERT_DIV;//界面能显示的值的范围
    double minValue=-valueRange/2-ui->sb_vert_offset->value();//界面能显示的最小值

    //依次绘制曲线
    for(int varIndex=0;varIndex<varList->size();varIndex++)
    {
        VarInfo &var=(*varList)[varIndex];
        if(!var.enableScope || var.samples.size()<2)//若不使能绘制或采样点太少则跳过
            continue;
        painter.setPen(QPen(var.lineColor,varIndex==chosenVarIndex?4:2,Qt::SolidLine));//设置笔刷（若是被选中的曲线则加粗）
        SamplePoint lastSamp=var.samples.at(0);//用于记录线段的上一个点
        for(int sampIndex=1;sampIndex<var.samples.size();sampIndex++)//依次连接采样点
        {
            SamplePoint thisSamp=var.samples.at(sampIndex);
            double lastTime=lastSamp.timestamp/1000000.0;
            double thisTime=thisSamp.timestamp/1000000.0;
            if(lastTime>startTime&&lastTime<endTime && thisTime>startTime&&thisTime<endTime && thisTime-lastTime<0.5)
            {
                QPoint lastPoint((lastTime-startTime)/timeRange*wid+posx,hei-(lastSamp.value-minValue)/valueRange*hei+posy);
                QPoint thisPoint((thisTime-startTime)/timeRange*wid+posx,hei-(thisSamp.value-minValue)/valueRange*hei+posy);
                if(!( (lastPoint.y()<posy && thisPoint.y()<posy) || (lastPoint.y()>posy+hei && thisPoint.y()>posy+hei) ))
                    painter.drawLine(lastPoint,thisPoint);
            }
            lastSamp=thisSamp;
        }
    }

    if(looking)//若正在查看变量则绘制查看线
    {
        painter.setPen(QPen(QColor(184,184,184),3,Qt::SolidLine));
        painter.drawLine(mousePos.x()-offsetx,posy,mousePos.x()-offsetx,posy+hei);
    }

    //计算并显示选中变量的变量名、当前值、查看值
    if(chosenVarIndex>=0 && chosenVarIndex<varList->size() && varList->at(chosenVarIndex).samples.size()>=2)
    {
        const VarInfo *chosenVar=&varList->at(chosenVarIndex);
        ui->lab_name->setText("变量名:"+chosenVar->name);
        ui->lab_curval->setText(QString("当前值:%1").arg(chosenVar->samples.last().value));
        if(looking)//计算查看位置
        {
            double lookTime=(mousePos.x()-posx-offsetx)*1.0/wid*timeRange+startTime;
            for(int sampIndex=0;sampIndex<chosenVar->samples.size()-1;sampIndex++)//依次检查采样点找到时间戳与查看位置最接近的点
            {
                if(chosenVar->samples.at(sampIndex).timestamp/1000000.0<=lookTime&&
                        chosenVar->samples.at(sampIndex+1).timestamp/1000000.0>=lookTime)
                {
                    ui->lab_lookval->setText(QString("查看值:%1").arg(chosenVar->samples.at(sampIndex).value));
                    break;
                }
                if(sampIndex==chosenVar->samples.size()-2)
                    ui->lab_lookval->setText("");
            }
        }
        else
        {
            ui->lab_lookval->setText("");
        }
    }
    else
    {
        ui->lab_name->setText("");
        ui->lab_curval->setText("");
        ui->lab_lookval->setText("");
    }
}

//重写事件过滤器，用于触发绘图label的绘图事件
bool GraphWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched==ui->txt_graph && event->type()==QEvent::Paint)
    {
        paintGraph(ui->txt_graph);
        return true;
    }
    return QDialog::eventFilter(watched,event);
}

//滚轮事件
void GraphWindow::wheelEvent(QWheelEvent *event)
{
    if(shiftFlag) //shift按下则竖直缩放
    {
        float dist=ui->sb_vert_rsv->value()*0.01; //最大/最小值增减量
        if(event->delta()>0)
        {
            ui->sb_vert_rsv->setValue(ui->sb_vert_rsv->value()-dist);
        }
        else if(event->delta()<0)
        {
            ui->sb_vert_rsv->setValue(ui->sb_vert_rsv->value()+dist);
        }
    }
    if(ctrlFlag) //ctrl按下则水平伸缩
    {
        float dist=ui->sb_hori_rsv->value()*0.01; //横向分辨率增减量
        int barDist=dist*1000*HORI_DIV/2;
        if(event->delta()>0)
        {
            if(ui->sb_hori_rsv->value()>0.001)
            {
                ui->sb_hori_rsv->setValue(ui->sb_hori_rsv->value()-dist);
                if(ui->bar_hori->value()<ui->bar_hori->maximum()-barDist)
                    ui->bar_hori->setValue(ui->bar_hori->value()+barDist);
            }
        }
        if(event->delta()<0)
        {
            ui->sb_hori_rsv->setValue(ui->sb_hori_rsv->value()+dist);
            if(ui->bar_hori->value()>ui->bar_hori->minimum()+barDist)
                ui->bar_hori->setValue(ui->bar_hori->value()-barDist);
        }
    }
    if(!ctrlFlag && !shiftFlag && !altFlag) //无任何组合键按下则时间轴左右滚动
    {
        float dist=ui->sb_hori_rsv->value()*100; //水平拖动条位置增减量
        if(event->delta()>0 && ui->bar_hori->value()>0)
        {
            if(ui->bar_hori->value()>dist)
                ui->bar_hori->setValue(ui->bar_hori->value()-dist);
            else
                ui->bar_hori->setValue(0);
        }
        else if(event->delta()<0 && ui->bar_hori->value()<ui->bar_hori->maximum())
        {
            if(ui->bar_hori->maximum()-ui->bar_hori->value()>dist)
                ui->bar_hori->setValue(ui->bar_hori->value()+dist);
            else
                ui->bar_hori->setValue(ui->bar_hori->maximum());
        }

    }
}

//按键按下事件
void GraphWindow::keyPressEvent(QKeyEvent *event)
{
    //三种组合键按下时对应标记置true
    if(event->key()==Qt::Key_Control)
        ctrlFlag=true;
    else if(event->key()==Qt::Key_Shift)
        shiftFlag=true;
    else if(event->key()==Qt::Key_Alt)
        altFlag=true;
    updateCursor();
}

//按键松开事件
void GraphWindow::keyReleaseEvent(QKeyEvent *event)
{
    //三种组合键松开时对应标记置false
    if(event->key()==Qt::Key_Control)
        ctrlFlag=false;
    else if(event->key()==Qt::Key_Shift)
        shiftFlag=false;
    else if(event->key()==Qt::Key_Alt)
        altFlag=false;
    updateCursor();
}

//鼠标按下事件
void GraphWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton) //如果左键按下则开启查看（ALT未按下）或拖拽（ALT按下）
    {
        if(altFlag)
            dragging=true;
        else
            looking=true;
        mousePos.setX(event->x()); //记录鼠标坐标
        mousePos.setY(event->y());
        lastMousePos=mousePos;
    }
}

//鼠标松开事件
void GraphWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton) //如果左键松开则关闭查看和拖拽
        looking=dragging=false;
}

//鼠标移动事件（仅按下时触发）
void GraphWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(looking||dragging) //若在查看或拖拽状态则记录鼠标位置
    {
        mousePos.setX(event->x());
        mousePos.setY(event->y());
    }
}

//定时器槽函数，触发绘图
void GraphWindow::onTrig()
{
    ui->txt_graph->update();
}

//用于外部设置变量列表
void GraphWindow::setVarList(QList<VarInfo> *list)
{
    varList=list;
}

//用于外部设置选中的变量
void GraphWindow::setChosenIndex(int varIndex)
{
    chosenVarIndex=varIndex;
}

//点击按钮时弹出说明界面
void GraphWindow::on_btn_help_clicked()
{
    //弹出messagebox显示帮助信息
    QString str="设置说明\n"
                "水平分辨率：水平方向每两根标线间表示的秒数\n"
                "竖直分辨率：竖直方向每两根标线间表示的范围\n"
                "实时更新：始终显示时间轴末端，跟随采样值的更新而变化\n\n"
                "快捷操作\n"
                "1.无按键+滚轮：时间轴左右滚动\n"
                "2.Ctrl+滚轮：图像水平缩放\n"
                "3.Shift+滚轮：图像竖直缩放\n"
                "4.Alt+左键拖拽：移动图像\n\n"
                "查看操作\n"
                "按住鼠标左键并拖动可以查看所选变量曲线的值（主界面单击变量名可选中变量）";
    QMessageBox box;
    box.setWindowTitle("绘图界面说明");
    box.setText(str);
    box.exec();
}

//根据各标志更新光标形状
void GraphWindow::updateCursor()
{
    if(ctrlFlag&&shiftFlag)//ctrl和shift同时按下，水平竖直方向同步缩放，显示斜向缩放箭头
        ui->txt_graph->setCursor(Qt::SizeFDiagCursor);
    else if(ctrlFlag)//ctrl按下，水平方向缩放，显示横向箭头
        ui->txt_graph->setCursor(Qt::SizeHorCursor);
    else if(shiftFlag)//shift按下，纵向缩放，显示纵向箭头
        ui->txt_graph->setCursor(Qt::SizeVerCursor);
    else if(altFlag)//alt按下，可以拖拽图像，显示拖动光标
        ui->txt_graph->setCursor(Qt::SizeAllCursor);
    else //没有按键按下，显示十字光标
        ui->txt_graph->setCursor(Qt::CrossCursor);
}
