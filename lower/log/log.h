#ifndef _LOG_H_
#define _LOG_H_

/**************����������**************/

//��׼��ͷ�ļ�
//����ϣ��ʹ�ö�Ӧ������ע��ͷ�ļ����޸��·�������ʵ����Ӧ����
#include <stdio.h> //ʹ��sprintf
#include <string.h> //ʹ��memcpy��strlen
//��׼�⺯����ֲ��
#define LOG_MEMCPY(dst,src,len) memcpy(dst,src,len) //�ڴ濽��
#define LOG_SPRINTF(buf,fmt,...) sprintf(buf,fmt,##__VA_ARGS__) //��ʽ���ַ���
#define LOG_STRLEN(str) strlen(str) //�����ַ�������

//��־��������С���ܴ�СΪ��������ֵ�ĳ˻�
#define LOG_MAX_LEN 100 //������־��������С��������־���ݡ�ʱ���������������Ϣ�����鲻С��100
#define LOG_MAX_QUEUE_SIZE 10 //�������ɴ���־����

//��ȡʱ����ӿ�(�����󾭹��ĺ�����)
#define LOG_GET_MS() HAL_GetTick() //����ʹ��STM32-HAL

//��ȡ������ں������ĺ�(һ���ɱ������ṩ)
#define LOG_GET_FUNC_NAME() (__FUNCTION__)

//�Ƿ�����־���(ע�͵���������������滻Ϊ�����)
#define LOG_ENABLE

/**************����������**************/

/**************����־����ӿڡ�**************/
//��Ϣ��־
#define LOG_INFO(tag,msg,...) LOG_ADD_FORMAT('i',(tag),(msg),##__VA_ARGS__)
//������־
#define LOG_DEBUG(tag,msg,...) LOG_ADD_FORMAT('d',(tag),(msg),##__VA_ARGS__)
//������־
#define LOG_WARN(tag,msg,...) LOG_ADD_FORMAT('w',(tag),(msg),##__VA_ARGS__)
//������־
#define LOG_ERROR(tag,msg,...) LOG_ADD_FORMAT('e',(tag),(msg),##__VA_ARGS__)
/**************����־����ӿڡ�**************/

//��־���������ͣ��Զ��з�ʽ�洢
typedef struct{
	int maxSize; //������󳤶ȣ���ʼ��ʱ�븳ֵΪLOG_MAX_QUEUE_SIZE
	char buf[LOG_MAX_QUEUE_SIZE][LOG_MAX_LEN]; //ʵ�ʴ洢�ռ�
	int lenBuf[LOG_MAX_QUEUE_SIZE]; //��bufһһ��Ӧ����ʾ�洢����־����
	int size,startPos;//��ǰ���г��ȺͶ�ͷλ��
}LogQueue;

#ifdef LOG_ENABLE
//��־������д������һ����־��������
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
//�������������壬ʵ�ʶ����������һ��Դ�ļ���
extern LogQueue logQueue;
#else
#define LOG_ADD_FORMAT(attr,tag,msg,...)
#endif

#endif
