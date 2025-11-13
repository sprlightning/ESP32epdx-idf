#include "EPD_SPI.h"
#include "NeverUseArduino.h"

#ifndef vspi_handle
    spi_device_handle_t vspi_handle = NULL; // 定义并初始化SPI设备句柄
#else
    extern spi_device_handle_t vspi_handle;
#endif

//SPI write byte - SPI发送（1位）数据
void SPI_Write(unsigned char value)
{				   			 
//    SPI.transfer(value);
    spi_transaction_t transcation = {
        .length = 8,           // 数据长度（1位）
        .tx_buffer = &value,   // 发送缓冲区（指针）
		.rx_buffer = NULL,     // 接收缓冲区（无）
    };
    esp_err_t ret = spi_device_polling_transmit(vspi_handle, &transcation);
    if (ret != ESP_OK)
    {
        printf("SPI write error: %d\n", ret);
    }
}

//SPI write command
void EPD_W21_WriteCMD(unsigned char command)
{
	EPD_W21_CS_0;
	EPD_W21_DC_0;  // D/C#   0:command  1:data  
	SPI_Write(command);
	EPD_W21_CS_1;
}
//SPI write data
void EPD_W21_WriteDATA(unsigned char datas)
{
	EPD_W21_CS_0;
	EPD_W21_DC_1;  // D/C#   0:command  1:data
	SPI_Write(datas);
	EPD_W21_CS_1;
}


// Define SPI pins
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_SCK  18
#define SPI_CS   5
// Save original SPI settings
// SPIClass* originalSPI = &SPI;
uint8_t originalSdaPin = SPI_MISO;
uint8_t originalSclPin = SPI_SCK;
bool spiInitialized = false;
unsigned char EPD_W21_ReadDATA(void)
{
    unsigned char i, j = 0;
    // Save current hardware SPI configuration (only on first call)
    if (!spiInitialized) {
        spiInitialized = true;
    }
    // originalSPI->end();    // Disable hardware SPI
    if (vspi_handle != NULL)
    {
        spi_bus_remove_device(vspi_handle); // 仅移除当前设备
        // spi_bus_free(_spi_host); // 不建议使用，这会释放整个SPI总线
        vspi_handle = NULL;
    }
    // Configure software SPI pins
    pinMode(SPI_MOSI, INPUT);     // SDA IN 
    pinMode(SPI_SCK, OUTPUT);    // SCL OUT 
    EPD_W21_CS_0;                   
    EPD_W21_DC_1;           // Data mode
    // Software SPI read
    for (i = 0; i < 8; i++) {
        digitalWrite(SPI_SCK, HIGH);  // SCL HIGH
        delayMicroseconds(2);    // More precise timing
        j = (j << 1);
        if (digitalRead(23) == HIGH)
            j |= 0x01;
        delayMicroseconds(2);
        digitalWrite(SPI_SCK, LOW);   // SCL LOW
    } 
    EPD_W21_CS_1; 
    pinMode(SPI_MOSI, OUTPUT);// Restore MOSI to output mode 
    // Re-enable hardware SPI
    // Reinitialize with predefined default pins
    // originalSPI->begin(originalSclPin, originalSdaPin, SPI_MOSI, SPI_CS);
    // 初始化SPI总线
    if (vspi_handle == NULL)
    {
        // 配置SPI总线
        spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = -1, //  Eink通常不需要MISO，设为-1
        .sclk_io_num = SPI_SCK,
        .quadwp_io_num = -1,     // 未使用Quad SPI
        .quadhd_io_num = -1,     // 未使用Quad SPI
        .max_transfer_sz = 4096, // 根据屏幕需求调整
        .flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0,
        };
        if (!spiInitialized) // 判断目标SPI总线是否已经初始化
        {
        esp_err_t ret = spi_bus_initialize(VSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) // 允许总线已初始化的情况
        {
            ESP_ERROR_CHECK(ret);
        }
        spiInitialized = true;
        }
        // 配置SPI设备
        spi_device_interface_config_t dev_cfg = {
        .command_bits = 0,           // 命令位数量（0表示无命令位）
        .address_bits = 0,           // 地址位数量（0表示无地址位）
        .dummy_bits = 0,             // dummy位数量
        .mode = 0,                   // SPI模式（0-3），通常是0
        .duty_cycle_pos = 128,       // 时钟占空比
        .cs_ena_pretrans = 0,        // 传输前片选建立时间
        .cs_ena_posttrans = 1,       // 传输后保持CS有效1个SPI时钟周期
        .clock_speed_hz = 10000000,  // 时钟频率（Hz）:10MHz
        .input_delay_ns = 0,         // 输入延迟（纳秒）
        .spics_io_num = SPI_CS,      // 自动控制CS引脚
        .flags = 0,                  // 标志位
        .queue_size = 7,             // 事务队列大小
        .pre_cb = NULL,              // 传输前回调函数
        .post_cb = NULL,             // 传输后回调函数
        };
        ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev_cfg, &vspi_handle));
    }
    return j;
}