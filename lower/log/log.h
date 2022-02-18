#ifndef _LOG_H_
#define _LOG_H_

/**************↓配置区↓**************/

//标准库头文件
//若不希望使用对应函数可注释头文件后修改下方函数宏实现相应功能
#include <stdio.h> //使用sprintf
#include <string.h> //使用memcpy、strlen
//标准库函数移植宏
#define LOG_MEMCPY(dst,src,len) memcpy(dst,src,len) //内存拷贝
#define LOG_SPRINTF(buf,fmt,...) sprintf(buf,fmt,##__VA_ARGS__) //格式化字符串
#define LOG_STRLEN(str) strlen(str) //计算字符串长度

//日志缓冲区大小，总大小为下列两个值的乘积
#define LOG_MAX_LEN 100 //单条日志缓冲区大小，包含日志内容、时间戳、函数名等信息，建议不小于100
#define LOG_MAX_QUEUE_SIZE 10 //缓冲区可存日志条数

//获取时间戳接口(启动后经过的毫秒数)
#define LOG_GET_MS() HAL_GetTick() //本例使用STM32-HAL

//获取语句所在函数名的宏(一般由编译器提供)
#define LOG_GET_FUNC_NAME() (__FUNCTION__)

//是否开启日志输出(注释掉则所有输出语句会替换为空语句)
#define LOG_ENABLE

/**************↑配置区↑**************/

/**************↓日志输出接口↓**************/
//信息日志
#define LOG_INFO(tag,msg,...) LOG_ADD_FORMAT('i',(tag),(msg),##__VA_ARGS__)
//调试日志
#define LOG_DEBUG(tag,msg,...) LOG_ADD_FORMAT('d',(tag),(msg),##__VA_ARGS__)
//警告日志
#define LOG_WARN(tag,msg,...) LOG_ADD_FORMAT('w',(tag),(msg),##__VA_ARGS__)
//错误日志
#define LOG_ERROR(tag,msg,...) LOG_ADD_FORMAT('e',(tag),(msg),##__VA_ARGS__)
/**************↑日志输出接口↑**************/

//日志缓冲区类型，以队列方式存储
typedef struct{
	int maxSize; //队列最大长度，初始化时须赋值为LOG_MAX_QUEUE_SIZE
	char buf[LOG_MAX_QUEUE_SIZE][LOG_MAX_LEN]; //实际存储空间
	int lenBuf[LOG_MAX_QUEUE_SIZE]; //与buf一一对应，表示存储的日志长度
	int size,startPos;//当前队列长度和队头位置
}LogQueue;

#ifdef LOG_ENABLE
//日志输出集中处理，添加一条日志到缓冲区
#define LOG_ADD_FORMAT(attr,tag,msg,...) if(logQueue.size<LOG_MAX_QUEUE_SIZE){ \
	int index=(logQueue.startPos+logQueue.size+1)%LOG_MAX_QUEUE_SIZE; \
	char *bufStartAddr=logQueue.buf[index],*buf=bufStartAddr; int termLen=0; \
	*buf=attr; buf+=1; \
	termLen=LOG_STRLEN(tag)+1; LOG_MEMCPY(buf,tag,termLen); buf+=termLen; \
	LOG_SPRINTF(buf,msg,##__VA_ARGS__); termLen=LOG_STRLEN(buf)+1; buf+=termLen; \
	LOG_SPRINTF(buf,"%d",LOG_GET_MS()); termLen=LOG_STRLEN(buf)+1; buf+=termLen; \
	termLen=LOG_STRLEN(LOG_GET_FUNC_NAME())+1; LOG_MEMCPY(buf,LOG_GET_FUNC_NAME(),termLen); buf+=termLen; buf+=termLen; \
	logQueue.lenBuf[index]=buf-bufStartAddr; logQueue.size++; \
}
//声明缓冲区定义，实际定义可在任意一个源文件中
extern LogQueue logQueue;
#else
#define LOG_ADD_FORMAT(attr,tag,msg,...)
#endif

#endif
