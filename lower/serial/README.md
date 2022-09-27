# 串口下位机程序说明

---

## 相关文件

本下位机程序仅包含单个文件`debug.c`，位于仓库`lower/serial`目录下，也可以下载发行版中的下位机程序压缩包后在`serial`文件夹中找到

---

## 配置项

* **`#define DEBUG_SEND(buf,len)`：需配置为所用平台的串口发送语句，将buf所指向的len个字节通过串口发出

* `#define DEBUG_RESET()`：需配置为所用平台的复位语句，上位机中点击复位并运行时会调用该语句

* `#define DEBUG_READ_ADDR_RANGE(addr)`：读地址限制条件，若请求的地址add不符合条件则返回0x00

* `#define DEBUG_WRITE_ADDR_RANGE(addr)`：写地址限制条件，若请求的地址addr不符合条件则不会写入 

> 注：带**的项为必需配置，其余项若不需要对应功能可不定义

---

## 函数接口

* `void Debug_SerialRecv(uint8_t *buf,uint16_t len);`：外部程序需要在收到串口数据时调用该函数进行解析，buf为数据首地址，len为字节数

	> 注：程序使用循环队列作为接收数据缓存，调用该函数时无需保证一次性传入完整数据帧，但应保证一个数据帧接收结束时及时调用该函数

---

## 移植说明

1. 开启一个串口，将下位机程序添加到项目工程中

	> 注：串口需配置为波特率115200、8位数据位，无校验位、1位停止位

2. 修改配置项

3. 在收到串口数据时调用`Debug_SerialRecv`函数

---

## 移植示例

### STM32 & HAL & 中断收发

```c

/****Debug.c****/
//...
//串口发送指令（使用串口1）
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_IT(&huart1,(buf),(len))
//复位指令
#define DEBUG_RESET() { \
	__set_FAULTMASK(1); \
	NVIC_SystemReset(); \
}
//...

/****main.c****/
//...
int main(void)
{
	//...(需先进行串口初始化)
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE); //使能串口RXNE中断
	//...
	while(1)
	{
		//...
	}
}
//...

/****stm32f1xx_it.c****/
//...
void USART1_IRQHandler(void) //串口中断服务函数
{
	if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_RXNE)!=RESET) //判定为RXNE中断
	{
		uint8_t ch=huart1.Instance->DR; //读出收到的字节
		Debug_SerialRecv(&ch,1); //进行解析
	}
}
//...

```

### Arduino 串口轮询方式

```c++

/****main.ino****/

//...
#define DEBUG_SEND(buf,len) Serial.write((buf),(len))
//...(下位机程序其他部分)

void setup()
{
	//...
	Serial.begin(115200); //初始化串口波特率为115200
	//...
}

void loop()
{
	while(Serial.available())
	{
		uint8_t ch=Serial.read(); //读出串口数据
		Debug_SerialRecv(&ch,1); //进行解析
	}
	//...其余代码不能有阻塞情况，需尽快执行完毕
}

```
