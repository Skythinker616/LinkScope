#ifndef VARTYPE_H
#define VARTYPE_H

//本头文件用于定义存放变量信息的结构

#include <QWidget>

//单个采样点
struct SamplePoint{
    double value;//采样数值
    qint64 timestamp;//时间戳
};

//单个变量的信息
struct VarInfo{
    QString name;//变量名
    QString rawValue;//有gdb获得的原始变量值
    bool enableScope;//是否使能绘图
    QList<SamplePoint> samples;//采样点列表
    QColor lineColor;//图线颜色
};

#endif // VARTYPE_H
