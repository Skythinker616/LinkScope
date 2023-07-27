# Log Program (Embedded Logger) - Program Guide

**[简体中文](README.md) | English**

---

## Related Files

This embedded logger program consists of two files: `log.c` and `log.h`. They are located in the `lower/log` directory of the repository. Alternatively, you can download the embedded logger program's release package and find them in the `log` folder.

---

## Configuration Options

* **Standard Library Function Configuration**

	* `#define LOG_MEMCPY(dst, src, len)`: Memory copy function. By default, it uses the `memcpy` function from `string.h`.

	* `#define LOG_SPRINTF(buf, fmt, ...)`: Formatted string printing function. By default, it uses the `sprintf` function from `stdio.h`.

	* `#define LOG_STRLEN(str)`: String length calculation function. By default, it uses the `strlen` function from `string.h`.

	> Note: The standard library function configuration is designed for situations where standard library functions are not supported on the platform. In general, modifications are unnecessary.

* **Buffer Size Configuration**

	* `#define LOG_MAX_LEN`: Maximum length of a single log data, including log tag, log content, timestamp, function name, etc. It is recommended to be no less than 100.

	* `#define LOG_MAX_QUEUE_SIZE`: Maximum number of log entries that the buffer can hold. The value should be determined based on the log printing frequency. The default value is 10.

	> Note: The total memory occupied by the buffer is the product of the above two settings. Writing multiple logs within a short time may cause buffer overflow, and subsequently, some logs will be discarded.

* **Other Configuration Options**

	* `#define LOG_GET_MS()`: Timestamp retrieval interface. It should return the number of milliseconds since system startup.

	* `#define LOG_GET_FUNC_NAME()`: Function to retrieve the name of the macro expansion location. Usually, it can be implemented using the `__FUNCTION__` macro provided by the compiler. If the platform does not support it, it can be replaced with an empty string `("")`.

	* `#define LOG_ENABLE`: Log output enable option. If commented out, all log output statements will be replaced with empty statements.

---

## Log Output API

* `#define LOG_INFO(tag, msg, ...)`: Outputs an information log. `tag` is the log tag, and `msg` is the log content. The log content can include format specifiers and is formatted with the subsequent variable arguments.

	> Example: `LOG_INFO("sys", "code=%d", 123); // Outputs a log with the tag "sys" and content "code=123"`

* `#define LOG_DEBUG(tag, msg, ...)`: Outputs a debug log. The usage is the same as information logs.

* `#define LOG_WARN(tag, msg, ...)`: Outputs a warning log. The usage is the same as information logs.

* `#define LOG_ERROR(tag, msg, ...)`: Outputs an error log. The usage is the same as information logs.

---

## Porting Guide

1. Add `log.h` and `log.c` to your project directory.

	> Note: If you don't want to add a separate `log.c` file or your compiler does not support multiple source files, you can include the content of `log.c` in any of your existing source files.

2. Modify configuration options according to your platform.

	* **In most cases, you only need to modify the timestamp configuration `LOG_GET_MS`**, and the other parameters can be left as default.

	* If your embedded device has limited memory or you want to output longer logs or more log entries at once, you can adjust the buffer size configuration `LOG_MAX_QUEUE_SIZE` and `LOG_MAX_LEN`.

	* If your compiler does not support the `__FUNCTION__` macro, replace `LOG_GET_FUNC_NAME` with an empty string.

	* If your compiler does not support standard library functions, you need to implement the functions yourself and replace them in the corresponding macro definitions.

3. In the files where you want to use logs, include `log.h`, and then call the log output interfaces. The upper-level system will periodically check and remove data from the log buffer.

---

## Important Notes

* The log output frequency should not be consistently too high. The upper-level system reads logs at a speed of about 10 logs per second. Prolonged high-frequency log output can cause buffer overflow and result in log loss.

