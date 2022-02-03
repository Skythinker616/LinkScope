#include <stdint.h>

#include "usart.h" //����ʹ��STM32������ƽ̨�������ô�ͷ�ļ�

/**************����������**************/
//���ڷ�����䣬��ʵ�ֽ�bufָ���len���ֽ�ͨ�����ڷ���
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_IT(&huart1,(buf),(len))
//��λָ����豸��֧�ֻ�����˹��ܿɲ�����
#define DEBUG_RESET() { \
	__set_FAULTMASK(1); \
	NVIC_SystemReset(); \
}
//����ַ����������������ĵ�ַaddr�����������򷵻�0x00���������ƿɲ�����
#define DEBUG_READ_ADDR_RANGE(addr) (addr>=0x20000000 && addr<=0x20001000)
//д��ַ����������������ĵ�ַaddr�����������򲻻�д�룬�������ƿɲ�����
#define DEBUG_WRITE_ADDR_RANGE(addr) (addr>=0x20000000 && addr<=0x20001000)

/**************����������**************/

//���ջ�������ѭ�����У���С���������޸�
#define DEBUG_RXBUF_SIZE 30
//���ͻ�������С���������޸�
#define DEBUG_TXBUF_SIZE 30

//����֡��ʽ��֡ͷ1B+֡��1B+������1B+����
//�̶�֡ͷ
#define DEBUG_FRAME_HEADER 0xDB
//������ö�٣����ڱ������֡����
typedef enum{
	SerialCMD_ReadMem, //���ڴ�����
	SerialCMD_WriteMem, //д�ڴ�����
	SerialCMD_Reset //��λ
}SerialCMD;

//���ڷ��ͻ�����
uint8_t debugTxBuf[DEBUG_TXBUF_SIZE];

//����ʵ����һ��ѭ���������ڴ��ڽ��գ���������
struct{
	uint8_t buf[DEBUG_RXBUF_SIZE]; //������
	uint16_t startPos,endPos; //��ͷ��βָ��
}debugRxQueue={0};
//���һ���ַ�
#define DEBUG_QUEUE_PUSH(ch) { \
	debugRxQueue.buf[debugRxQueue.endPos++]=(ch); \
	if(debugRxQueue.endPos>=DEBUG_RXBUF_SIZE) \
		debugRxQueue.endPos-=DEBUG_RXBUF_SIZE; \
}
//����һ���ַ�
#define DEBUG_QUEUE_POP() { \
	debugRxQueue.startPos++; \
	if(debugRxQueue.startPos>=DEBUG_RXBUF_SIZE) \
		debugRxQueue.startPos-=DEBUG_RXBUF_SIZE; \
}
//��ȡ��ͷ�ַ�
#define DEBUG_QUEUE_TOP() (debugRxQueue.buf[debugRxQueue.startPos])
//��ȡ���д�С
#define DEBUG_QUEUE_SIZE() \
	(debugRxQueue.startPos<=debugRxQueue.endPos? \
	debugRxQueue.endPos-debugRxQueue.startPos: \
	debugRxQueue.endPos+DEBUG_RXBUF_SIZE-debugRxQueue.startPos)
//��ȡ���е�pos��Ԫ��
#define DEBUG_QUEUE_AT(pos) \
	(debugRxQueue.startPos+(pos)<DEBUG_RXBUF_SIZE? \
	debugRxQueue.buf[debugRxQueue.startPos+(pos)]: \
	debugRxQueue.buf[debugRxQueue.startPos+(pos)-DEBUG_RXBUF_SIZE])

//��������
void Debug_SerialRecv(uint8_t *buf,uint16_t len);
void Debug_ParseBuffer(void);

//�����յ����ݺ��뱾�������н������豻�ⲿ����
void Debug_SerialRecv(uint8_t *buf,uint16_t len)
{
	for(uint16_t i=0;i<len;i++) //���յ��������������
		DEBUG_QUEUE_PUSH(buf[i]);
	Debug_ParseBuffer(); //�������
}

//������������
void Debug_ParseBuffer()
{
	if(DEBUG_QUEUE_AT(0)==DEBUG_FRAME_HEADER) //��һ���ֽ�Ϊ֡ͷ�����Լ�������
	{
		if(DEBUG_QUEUE_SIZE()>2 && DEBUG_QUEUE_SIZE()>=DEBUG_QUEUE_AT(1)) //֡���㹻�����Խ���
		{
			uint16_t frameLen=DEBUG_QUEUE_AT(1);//����֡��
			uint8_t cmd=DEBUG_QUEUE_AT(2); //����������
			if(cmd==SerialCMD_ReadMem) //��Ҫ��ȡ�ڴ�����
			{
				uint8_t byteNum=DEBUG_QUEUE_AT(3); //Ҫ��ȡ���ֽ���
				uint32_t addr=0; //����Ŀ���ַ
				for(uint8_t i=0;i<4;i++)
					addr|=((uint32_t)DEBUG_QUEUE_AT(4+i))<<(i*8);
				debugTxBuf[0]=DEBUG_FRAME_HEADER; //������������֡
				debugTxBuf[1]=byteNum+3;
				debugTxBuf[2]=SerialCMD_ReadMem;
				for(uint8_t i=0;i<byteNum;i++) //����д��ָ����ַ������
				{
					uint8_t byte=0;
					#ifdef DEBUG_READ_ADDR_RANGE
					if(DEBUG_READ_ADDR_RANGE((addr+i)))
					#endif
						byte=*(uint8_t*)(addr+i);
					debugTxBuf[i+3]=byte;
				}
				DEBUG_SEND(debugTxBuf,byteNum+3); //���ڷ���
				for(uint8_t i=0;i<frameLen;i++) //����֡����
					DEBUG_QUEUE_POP();
			}
			else if(cmd==SerialCMD_WriteMem) //��Ҫд���ڴ�����
			{
				uint8_t byteNum=frameLen-7; //Ҫд����ֽ���
				uint32_t addr=0; //����Ŀ���ַ
				for(uint8_t i=0;i<4;i++)
					addr|=((uint32_t)DEBUG_QUEUE_AT(3+i))<<(i*8);
				for(uint8_t i=0;i<byteNum;i++) //����д������
				{
					#ifdef DEBUG_WRITE_ADDR_RANGE
					if(DEBUG_WRITE_ADDR_RANGE((addr+i)))
					#endif
						*(uint8_t*)(addr+i)=DEBUG_QUEUE_AT(7+i);
				}
				for(uint8_t i=0;i<frameLen;i++) //����֡����
					DEBUG_QUEUE_POP();
			}
			else if(cmd==SerialCMD_Reset) //��Ҫ��λ
			{
				#ifdef DEBUG_RESET
				DEBUG_RESET();
				#endif
			}
			if(DEBUG_QUEUE_SIZE()>0) //�����滹�����ݣ����еݹ����
				Debug_ParseBuffer();
		}
	}
	else //����֡����
	{
		while(DEBUG_QUEUE_AT(0)!=DEBUG_FRAME_HEADER && DEBUG_QUEUE_SIZE()>0) //���������ݳ���
			DEBUG_QUEUE_POP();
		if(DEBUG_QUEUE_SIZE()>0) //�����滹�����ݣ���������
			Debug_ParseBuffer();
	}
}
