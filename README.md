# ESP32epdx-idf
Use GoodDisplay's ePaper drivers on ESP-IDF.

## 基本信息

此例程及其驱动是[GoodDisplay](https://www.good-display.cn/)基于Arduino进行开发的，例程配套的开发板为[ESP32开发套件Eink开发板 ESP32-L系列](https://www.good-display.cn/product/323.html)，配套的转接板为[SPI电子纸转接板（带前置光、触控、SD 卡功能）| 兼容树莓派与ESP32 ESP32-FTS02](https://www.good-display.cn/product/664.html)；最新版驱动可访问GoodDisplay开源库[ESP32epdx](https://github.com/gooddisplayshare/ESP32epdx)获取；

我对2025-11月版本的驱动进行了改动，使其适配ESP-IDF 5.1.4环境，并补充了对GoodDisplay ESP32-L的IO定义，并修复了一些影响使用的bug，如文件编码不统一、中文字符错乱、不能正常显示中文字符串等；

其中[微雪电子](https://www.waveshare.net/)墨水屏驱动IDF适配版的开源地址是：[esp32-waveshare-epd-idf](https://github.com/sprlightning/esp32-waveshare-epd-idf)，经过对比，二者大部分相似，但GD的内容比微雪的较新；从bug上来说，微雪的驱动相对稳定。

我使用的是GoodDisplay的2.13寸触摸屏（GDEY0213B74）和其配套的ESP32-L开发板，在使用GD的[GDEY0213B74驱动](https://github.com/gooddisplayshare/ESP32epdx/tree/main/examples/2-Colors%20(BW)/2.13/GDEY0213B74)时遇到的bug包括无法**正常显示中文字符串**，如“你好abc”会显示成“你  abc”，但是微雪的驱动却可以正常显示，我仔细对比了两边的驱动，发现微雪的中文字符串驱动比GD多对比了一组数据，也就是截止符，同样对应的字符索引应当+1，这是微雪能正常显示中文字符串的关键点；此外GD驱动的bug还有各种初始化函数调度混乱，存在过多不必要的休眠，还有就是刷新显示的时候黑白屏闪烁的情况相对严重，起初以为是过多的RST导致，但后来排除了这个原因，还是涉及到底层寄存器的操作，暂没深入研究。

微雪显示时钟是调用函数**Paint_DrawTime**，其本质是将输入的时间转化为字符串存入一个简单的时钟数据结构，然后将时钟数据结构的内容通过英文字符串显示函数在局部进行更新；而GD驱动的一个好处是其有一个新增的时钟函数**EPD_Dis_Part_Time**，其通过调用新增的**EPD_Dis_Part_RAM**在局部刷新写入数据，可以非常流畅且连贯的更新时钟显示，时钟显示期间屏幕稳定且不闪烁，但是我仔细分析这个函数，发现它其实是预定义了一组数字的Image data，传入的数字会转化为Image data，本质上是在局部更新Image。而这个Image data存在于GD的Image.h头文件，本来想将GD的时钟函数移植到微雪的，但发现其显示的尺寸有点奇怪，试图显示较小的时钟尺寸时会发生字符错位，这种尺寸不通用的情况使我放弃了这个移植计划。当然这也表明了一件事，图片data的刷新可以做到屏幕不闪烁。但是这不是根本问题，我注意到同样的屏幕，微雪驱动在更新字符的时候却可以做到屏幕不闪烁，这是GD的驱动的bug之一，局部更新字符会出现屏幕黑白交替闪烁。

此外之前试图使用[GxEPD2](https://github.com/ZinggJM/GxEPD2)，这是个同时集成了GoodDisplay和微雪电子的ePaper驱动库，可是其高度依赖一个Arduino图形库GFX，那个GFX图形库移植较复杂，而我因为某些原因只能使用纯ESP-IDF环境开发，这使我放弃了这个GxEPD2驱动库。

## 基本使用

### Step 1

将整个ESP32epdx-idf文件夹复制到你的IDF项目的components文件夹中（或者用git获取），在项目main目录的CMakeLists.txt中添加对ESP32epdx-idf的依赖；值得注意的是，本项目的前置需求是[pschatzmann](https://github.com/pschatzmann)的[arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools)，其内置的NoArduino.h使用ESP-IDF方法重构了Arduino基本函数，使得非Arduino项目能在不引入Arduino组件的情况下使用Arduino的基本函数，例如常用的延时、IO操作、串口打印等；下面是main目录的CMakeLists.txt示例：

```cmake
    idf_component_register(SRCS "main.cpp"
                            INCLUDE_DIRS "."
                            REQUIRES arduino-audio-tools ESP32epdx-idf)
```

### Step 2

GD的每一款屏幕/总成都有对应的编号，如“GDEY0213B74”，根据你的屏幕编号修改ESP32epdx-idf的CMakeLists.txt的**EPD_DEMO_PATH**，默认使用的是"examples/2-Colors-BW/2.13/GDEY0213B74"，这将适用于GoodDisplay的2.13寸触摸屏（GDEY0213B74）；

### Step 3

参考demo的ino文件，将其所用的头文件放到到你的main.cpp中，demo的setup()的内容放到main.cpp的app_main()中即可，如果loop()有内容，那就创建一个task()，把loop()的内容放到task()里，并在app_main()末尾调用启动任务调度器启动此task()即可。

### 注意事项

在src/NverUseArduino.h中有两个开关，**NoArduino**和**USE_GxEPD**；

开关**NoArduino**控制**是否不使用Arduino**，默认置1是不使用Arduino，其会调用arduino-audio-tools的NoArduino.h，否则会调用Arduino.h；

开关**USE_GxEPD**控制**是否使用GoodDisplay的ESP32-L开发板**，其默认置1是启用GoodDisplay ESP32-L，如使用其它开发板如微雪的[电子墨水屏无线网络驱动板](https://www.waveshare.net/shop/e-Paper-ESP32-Driver-Board.htm)，需要将开关**USE_GxEPD**置0，否则会影响SPI和墨水屏控制IO的定义。同样的，如果你没有使用上述开发板，使用的是自定义板子，那请修改为实际IO值；

注意到墨水屏在sleep后没法通过初始化唤醒，会停留在“e-Paper busy”，是因为唤醒需要分4步，先重置-再初始化-再加载图像cache-再创建并选择图像cache，我在EPD_2in13_V4.cpp预留了一个被注释的Awake Function，可根据需要将其复制到main.cpp中来启用；因为唤醒需要处理图像cache，所以需要在main.cpp的全局变量提前定义图像cache。示例如下：
```cpp
/******************************************************************************
function :	Awake EPD from sleep mode
parameter:
info     :  This is an example of awake EPD, copy it to your main.cpp as a function
******************************************************************************/
void EPD_2in13_V4_Awake(void) {
    Debug("Awake EPD...\r\n");
    
    // reset EPD
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    
    // reinitialize EPD
    EPD_2in13_V4_Init();
    
    // reload image cache(need to define the image cache in your main.cpp as global variable)
    if (BlackImage == NULL) {
        if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) 
        {
            Debug("Failed to apply for black memory...\r\n");
            while (1);
        }
    }
    Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 270, WHITE);
    Paint_SelectImage(BlackImage);
}
```

## 字体

我在src/Fonts目录中增加了CJK字体，适用于大多数使用场景，可用**Paint_DrawString_CN**函数绘制；

使用STC-ISP的字库生成工具制作，水平扫描，从左到右从上到下，高位在前；

其中**Font12CJK**使用新宋体，12号，常规；点阵宽x高=16x21，字体宽x高=16x16；水平调整0，垂直调整2；

**Font12CJK_B**使用新宋体，12号，粗体；点阵宽x高=16x21，字体宽x高=16x16；水平调整0，垂直调整2；

值得注意的是，字库占用空间较大，可能单个CJK就需要1MB空间，为此可参考我提供的**music_fonts_cjk.txt**按照**stc_isp_char_config.ini**定义的规则生成字库bin文件放在独立的flash中；或者确保ESP32 Flash的factory分区有足够的空间存放带字库的ESP32 Image，这通常意味着需要4MB及以上的ESP32 Flash；

SIC-ISP工具位于extras/Font-tools目录，版本6.96A，来自STC官网；解压后运行，其菜单的工具选项可看到“字库生成工具”。

## FT6336触摸驱动

与GoodDisplay的2.13寸屏幕配套的FT6336触摸屏驱动详见[mlx-ft6336-drivers](https://github.com/sprlightning/mlx-ft6336-drivers)，同样支持ESP-IDF 5.1.4环境。

## GoodDisplay版本更新记录

v1.0.8
Added 3.97-inch monochrome and 4-color options, updated 2.9/5.79/7.5/11.6-inch 4-color options, and added fast refresh function.
增加3.97寸单色、四色，更新2.9/5.79/7.5/11.6寸四色，增加快刷功能。

v1.0.7
Add BT and WiFi control e-paper display examples
增加BT和WiFi控制电子纸显示示例

v1.0.6
Add a 4-color e-paper driver for sizes and improve GUI functionality
增加4色电子纸驱动程序，完善GUI功能

v1.0.5
Add a 3-color e-paper driver for sizes and improve GUI functionality
增加3色电子纸驱动程序，完善GUI功能

v1.0.4
Add a 2-color e-paper driver for sizes under 7.5 inches and improve GUI functionality
增加2色电子纸驱动程序，7.5寸以上，完善GUI功能

v1.0.1
Add a 2-color e-paper driver for sizes under 7.5 inches and improve GUI functionality
增加2色电子纸驱动程序，7.5寸以下，完善GUI功能



