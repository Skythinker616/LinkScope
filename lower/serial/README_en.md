# Serial Downlink Program Instructions

**[简体中文](README.md) | English**

---

## Related Files

This downlink program consists of a single file `debug.c`, located in the repository's `lower/serial` directory. It can also be downloaded as part of the release package in the `serial` folder.

---

## Configuration Options

* **`#define DEBUG_SEND(buf,len)`:** Configure this with the platform-specific statement for sending data via the serial port. It will transmit the `len` bytes pointed to by `buf` through the serial port.

* `#define DEBUG_RESET()`: Configure this with the platform-specific statement for resetting the device. The statement will be called when the reset button is pressed on the upper computer and executed.

* `#define DEBUG_READ_ADDR_RANGE(addr)`: Address read restriction condition. If the requested address `addr` does not meet the condition, it returns `0x00`.

* `#define DEBUG_WRITE_ADDR_RANGE(addr)`: Address write restriction condition. If the requested address `addr` does not meet the condition, it will not write.

* `#define DEBUG_ADDR_OFFSET`: Read and write address offset. The downlink program will add this offset to the received instructions before performing read/write operations.

> Note: Items marked with ** are mandatory configurations. Other items can be left undefined if the corresponding functionality is not required.

---

## Function Interfaces

* `void Debug_SerialRecv(uint8_t *buf, uint16_t len);`: External programs need to call this function to parse data when receiving serial data. `buf` is the address of the data, and `len` is the number of bytes.

> Note: The program uses a circular queue as the receiving data buffer, automatically handling packet splitting and sticking. Therefore, it is not necessary to ensure that complete data frames are passed in at once, but this function should be called promptly when one data frame reception is complete.

---

## Porting Instructions

1. Enable a serial port and add the downlink program to the project.

2. Modify the configuration options accordingly.

3. Call the `Debug_SerialRecv` function when receiving serial data.

---

## Troubleshooting

**The upper computer shows a read timeout after clicking connect, but the downlink program is still running normally.**

* Check the serial connection, including serial port parameter configuration (baud rate, etc.), hardware wiring, and related downlink program to ensure that the downlink program can receive serial data and return results.

**The upper computer shows a read timeout after clicking connect, and the downlink program encounters abnormal operation (such as hardware errors or resets).**

* This phenomenon is caused by reading and writing addresses out of bounds. Please refer to the chip manual and write the allowed lower and upper address limits to `DEBUG_READ_ADDR_RANGE` and `DEBUG_WRITE_ADDR_RANGE` configuration options.

**The upper computer is connected properly, but the displayed values are incorrect.**

* This might be due to an offset between the actual address in the downlink program and the symbol file on the upper computer. Measure this offset and write it to the `DEBUG_ADDR_OFFSET` configuration option.

* Offset Measurement Method:

	* Choose any variable (let's assume it's `var`).
	
	* View the value of `&var` in the upper computer (which represents the address in the symbol file).
	
	* Print the value of `&var` in the downlink program (which represents the actual address in the downlink program).
	
	* Subtract the two values to get the offset (actual address - symbol file address).

---

## Porting Examples

### STM32 & HAL & Interrupt-Based Communication

```c

/****Debug.c****/
//...
// Serial sending command (using USART1)
#define DEBUG_SEND(buf,len) HAL_UART_Transmit_IT(&huart1, (buf), (len))
// Reset command
#define DEBUG_RESET() { \
	__set_FAULTMASK(1); \
	NVIC_SystemReset(); \
}
//...

/****main.c****/
//...
int main(void)
{
	//...(Serial port initialization should be done first)
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE); // Enable RXNE interrupt for the serial port
	//...
	while(1)
	{
		//...
	}
}
//...

/****stm32f1xx_it.c****/
//...
void USART1_IRQHandler(void) // Serial port interrupt service routine
{
	if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) // Check for RXNE interrupt
	{
		uint8_t ch = huart1.Instance->DR; // Read the received byte
		Debug_SerialRecv(&ch, 1); // Parse the data
	}
}
//...

```

### Arduino & Polling-based Communication

```c++


/****main.ino****/

//...
#define DEBUG_SEND(buf, len) Serial.write((buf), (len))
//...(Other parts of the downlink program)

void setup()
{
	//...
	Serial.begin(115200); // Initialize the serial port with a baud rate of 115200
	//...
}

void loop()
{
	while(Serial.available())
	{
		uint8_t ch = Serial.read(); // Read serial data
		Debug_SerialRecv(&ch, 1); // Parse the data
	}
	//...No blocking code allowed, finish execution as quickly as possible
}

```
