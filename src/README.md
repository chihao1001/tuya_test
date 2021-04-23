# Tuya MCU SDK Arduino Library 

[English](./README.md) | [中文](./README_zh.md) 

Tuya MCU SDK Arduino Library 基于涂鸦Wi-Fi 通用对接方案进行开发的，设备 MCU 通过串口与 Wi-Fi 模组连接实现设备联网。**涂鸦模组中的固件应为通用固件，通用固件支持9600，115200两种波特率自适应。开发前请仔细阅读该文档。**  

<div align='center'>
<img src="https://images.tuyacn.com/smart/shiliu_zone/Tuya_Arduino_library/mcu_wifi_connect.png"/>
</div>



## 文件介绍 

```bash
├── config.h	//配置文件，通过一些宏对mcu sdk内的功能进行添加和裁剪
├── examples	//例程存放文件夹 
├── keywords.txt
├── library.properties
├── README.md
└── src	//Tuya mcu sdk Arduino Library 存放文件夹 
    ├── Tuya.cpp	//用户会使用到的API接口
    ├── TuyaDataPoint.cpp //对DP点相关操作的类
    ├── TuyaDataPoint.h
    ├── TuyaDefs.h	//会用到的，用户不需改动的一些常量
    ├── Tuya.h
    ├── TuyaTools.cpp	//MCU SDK会使用到的一些工具
    ├── TuyaTools.h
    ├── TuyaUart.cpp	//对串口和数据缓存区处理的一些函数
    └── TuyaUart.h
```



## 重要函数 

在Arduino中使用该库进行编程开发时，必须在你的Arduino工程文件中包含 `Tuya.h` 头文件。

### 1、串口中断接收

涂鸦模组和MCU之间通过串口通信，所以为了及时接收到数据进行处理，串口接收应采用中断的方式，Arduino 中使用的是 `void serialEvent()` 函数。

开发时，请将下面的函数不要修改的复制到你的工程中：

```c
#include <tuya.h>
...
    
//Please copy this function to your project, do not change it.
void serialEvent()
{
    while (Serial.available())   //Read hardware serial port data at all times
    {
        tuya_uart.uart_receive_input(Serial.read());
        delay(2);
    }
    while (Serial.read() >= 0) {} //Clear serial port cache
}
...
```



### 2、相关信息初始化

在使用Tuya IoT 云平台的时候，Product ID（简称：PID） 关联了产品具体的功能点、App 控制面板、出货信息等所有跟这个产品相关的信息。

`unsigned char Tuya::init(unsigned char *pid, unsigned char *mcu_ver)` ，PID通过在 [Tuya IoT平台](https://iot.tuya.com/) 上创建产品获取，wifi类的长度一般为16，`mcu_ver` 这个参数是软件的版本号，如果你的mcu想要支持OTA的话，该信息也是非常重要的（当前版本的library暂不支持OTA功能）。

```c
#include <Tuya.h>

Tuya my_device;
...
void setup() 
{   
  Serial.begin(9600);
  ...
  my_device.init("xxxxxxxxxxxxxxxx", "1.0.0");//"xxxxxxxxxxxxxxxx":应为你在Tuya IoT 平台上获取的的PID，"1.0.0"应是当前软件的实际版本号
      
  ...
}
...
```



### 3、DP信息传入MCU SDK 

DP为 Data Point 的缩写，即数据点，偶尔被称为 DP 点，表示智能设备所具备的功能点。

+ 涂鸦智能将每个功能点抽象成数据点，数据点定义成不同的类型，例如布尔、枚举、数值等。
+ 数据点具备读写属性。例如，一个两路的开关，可以抽象成两个数据点，每个数据点类型为布尔型，可取值为 `True` 或 `False`。
+ 数据点可读写，读表示获取开关目前的值，写表示改变开关目前的值。

DPID:指定通信协议下 DP 事件的 ID。



MCU SDK需要知道你创建了哪些DP点，他们是什么类型的，通过 `void Tuya::set_dp_cmd_total(unsigned char dp_cmd_array[][2], unsigned char dp_cmd_num)` 函数传入MCU SDK。Tuya IoT 对于DP的类型分为6类：

```c
#define DP_TYPE_RAW     0x00    //RAW type
#define DP_TYPE_BOOL    0x01    //bool type
#define DP_TYPE_VALUE   0x02    //value type
#define DP_TYPE_STRING  0x03    //string type
#define DP_TYPE_ENUM    0x04    //enum type
#define DP_TYPE_BITMAP  0x05    //fault type
```

关于`void Tuya::set_dp_cmd_total(unsigned char dp_cmd_array[][2], unsigned char dp_cmd_num)` 函数，`dp_cmd_array[][2]` 为存储D信息的数组，`dp_cmd_num` 为DP总数。



这里我们假设有一个灯，它主要有3个功能也就是3个DP点，分别是开关（DP ID : 20, DP 类型：布尔类型），灯的亮度（DP ID : 20, DP 类型：数值类型），灯光模式（DP ID : 20, DP 类型：枚举类型）。

```c
#include <Tuya.h>

Tuya my_device;
...
#define DPID_SWITCH	20	//灯的开关DP
#define DPID_LIGHT	21	//灯的亮度DP
#define DPID_MODE	22	//灯的工作模式DP
    
//注意：array[][0]为DPID, array[][1]为DP type
unsigned char dp_id_array[][2] = {
    /*  DPID     |  DP type  */
    {DPID_SWITCH, DP_TYPE_BOOL},  
    {DPID_LIGHT, DP_TYPE_VALUE},
    {DPID_MODE, DP_TYPE_ENUM},
};
...
void setup() 
{
    ...
    my_device.set_dp_cmd_total(dp_id_array, 3);	
    ...
}
```



### 4、配网模式设置 

调用 `void Tuya::mcu_set_wifi_mode(unsigned char mode)` 进入配网模式。

```c
/**
 * @description: MCU set wifi working mode
 * @param {unsigned char} mode : enter mode 
 *                               0(SMART_CONFIG):enter smartconfig mode
 *                               1(AP_CONFIG):enter AP mode
 * @return {*}
 */
void Tuya::mcu_set_wifi_mode(unsigned char mode);
```





### 5、下发DP数据处理 

当云端下发数据后，需要对下发的数据进行解析，通过`unsigned char Tuya::mcu_get_dp_download_data(unsigned char dpid, const unsigned char value[], unsigned short len)` 这个函数进行解析。该函数目前只支持`DP_TYPE_BOOL`，`DP_TYPE_VALUE`，`DP_TYPE_ENUM` 这3种类型；`DP_TYPE_BITMAP` 为故障型一般只上报处理，不会下发，对该类型不用处理；`DP_TYPE_RAW` ,`DP_TYPE_STRING` 这两种类型需要用户自己进行处理。



```c
/**
 * @description: mcu gets bool,value,enum type to send dp value. (raw, string type needs to be handled at the user's discretion. fault only report)
 * @param {unsigned char} dpid : data point ID 
 * @param {const unsigned char} value : dp data buffer address 
 * @param {unsigned short} len : data length
 * @return {unsigned char} Parsed data
 */
unsigned char Tuya::mcu_get_dp_download_data(unsigned char dpid, const unsigned char value[], unsigned short len);
```



### 6、DP下发处理函数注册 

当APP控制设备的时候，会从云端下发对应的DP命令到设备，设备对数据进行解析后，对下发的命令执行相对于的动作。

对下发命令的处理，通过回调函数来调用，所以我们需要把你的处理函数注册一下。调用这个`void Tuya::dp_process_func_register(tuya_callback_dp_download _func)` 对回调函数进行注册。

```c
#include <Tuya.h>

Tuya my_device;
...

void setup() 
{
    ...
  //register DP download processing callback function
  my_device.dp_process_func_register(dp_process);
    ...
}

/**
 * @description: DP download callback function.
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} length
 * @return {unsigned char}
 */
unsigned char dp_process(unsigned char dpid,const unsigned char value[], unsigned short length)
{
  switch(dpid) {
    case DPID_SWITCH:
      switch_value = my_device.mcu_get_dp_download_data(dpid, value, length);
      if (switch_value) {
        //Turn on 

      } else {
        //Turn off

      }
      //Status changes should be reported.
      my_device.mcu_dp_update(dpid, value, length);
    break;

    default:break;
  }
  return SUCCESS;
}
```



### 7、上报设备状态 

上报设备状态也就是上报所有DP点值，也是通过注册函数的方式进行的。



接收 DataPoint 的 6 种不同DP类型的数据类型定义：

**raw型、字符型（string）应定义为数组(unsigned char array[])；**

**布尔型(bool)、枚举型(enum) 应定义为无符号整型(unsigned char value)；**

**数值型(value)、故障型(BITMAP) 应定义为无符号长整形(unsigned long value)** 

<font color='red'>**请严格遵守上面的类型定义，否则可能会出现一些问题。**</font>



DP上报函数：

```c
/**
 * @description: dp data upload
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} len
 * @return {*}
 */
unsigned char mcu_dp_update(unsigned char dpid, const unsigned char value[], unsigned short len);//update raw, string type
unsigned char mcu_dp_update(unsigned char dpid, unsigned char value);//update bool, enum type 
unsigned char mcu_dp_update(unsigned char dpid, unsigned long value);//update value, bitmap(fault) type
```



上报设备状态函数注册示例：

```c
#include <Tuya.h>

Tuya my_device;

#define DPID_SWITCH 20
//Record the current status of the led
unsigned char switch_value = 0;
...
void setup() 
{
    ...
  //register DP download processing callback function
  my_device.dp_update_all_func_register(dp_update_all);
    ...
}

/**
 * @description: Upload all DP status of the current device.
 * @param {*}
 * @return {*}
 */
void dp_update_all(void)
{
  my_device.mcu_dp_update(DPID_SWITCH, switch_value);
}
```





## 技术支持

您可以通过以下方法获得涂鸦的支持:

- 开发者中心：https://developer.tuya.com
- 帮助中心: https://support.tuya.com/cn/help
- 技术支持工单中心: https://service.console.tuya.com 