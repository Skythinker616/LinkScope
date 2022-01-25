# LinkScope

---

## 项目简介

本程序使用QT编写，基于OpenOCD和GDB，用于硬件设备的调试，有以下几个功能

* **实时查看**和**修改**变量值

* 变量值**波形实时绘制**

* 采样数据导出到CSV表格

* 采样频率约100Hz

程序理论上支持OpenOCD所支持的各种调试器及硬件芯片，如STLink、JLink、CMSIS-DAP等以及STM32全系列等

目前已测试STLink和CMSIS-DAP对STM32F103RCT6芯片的调试，未发现问题

![运行演示](imgs/run-demo.png)

---

## 使用方法

1. 在下拉框中选择调试器和芯片类型，选择Axf文件路径，点击连接即可尝试连接芯片

2. 在表格最后一行变量名处填写变量名可以添加查看变量，选中变量名按Del键可以删除变量

> 注：变量名不仅可以填入单个变量名，还可以填入合法的C语言表达式（GDB支持即可）；复合类型不能修改和绘图，只能实时查看

1. 编辑`修改变量`列可以修改变量值，双击`图线颜色`列可以选择绘图颜色

![基本操作](imgs/simp-oper.gif)

4. 单击`变量名`列选中对应的变量，绘图窗口会加粗绘制波形，左下角会显示当前值和查看值（拖动鼠标进行查看）

5. 绘图界面说明可以在绘图窗口点击操作说明查看，滚轮配合`Ctrl`、`Shift`、`Alt`可以实现画面的缩放和移动

![绘图操作](imgs/graph-oper.gif)

---

## 菜单项说明

* `显示绘图窗口`：手动关闭绘图窗口后可以通过这个菜单项重新打开绘图窗口并显示到前台

* `刷新连接配置`：连接配置文件位于`openocd/share/openocd/scripts`下的`target`和`interface`中，用户可按照OpenOCD语法编写配置脚本，放入对应目录下，然后点击该菜单项将配置文件加载到下拉选框中

* `保存配置`：软件中所配置的调试器型号、芯片型号、Axf文件路径和各变量的配置都可以通过该菜单项保存到一个配置文件中

* `导入配置`：将上述保存的配置文件重新载入软件中

* `导出数据`：将获取到的各变量采样数据导出到CSV表格文件

---

## 使用注意事项

* 若不指定Axf文件，无法使用变量名，只能通过绝对地址进行查看

* 修改Axf路径后需要重新连接

* 本程序不带下载功能，连接目标前请确认已为目标芯片下载过指定程序；若更换为不同类型的调试器，即使芯片程序没有变动，也应使用更换后的调试器再次下载程序

---

## 已知问题及解决方法

* 若程序发生错误闪退，可能在下次运行时无法成功连接目标，可以尝试手动查找`openocd.exe`进程并强制结束

---

## TODO

* 将采样频率提升至约1kHz

---

## 运行过程简介

* 连接目标时，程序会在后台启动OpenOCD进程进行连接，并命令GDB进程连接到OpenOCD

* 运行过程中程序会不断模拟与GDB进程进行命令行交互，在用户添加变量时使用`display expr`指令将变量添加到GDB的查看表中，同时定时10ms发送`display`指令并进行正则解码，更新用户界面

* 程序开有一个微秒级定时器，每收到一个变量采样数据时，会从该定时器获取当前的时间戳并与数据一起记录下来，同时绘图窗口会不断对历史数据进行更新绘图

---

## 仓库文件说明

* 编译程序后需要将`gdb`和`openocd`复制到可执行文件同级目录下
