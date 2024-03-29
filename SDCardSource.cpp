
/**
 * @file SDCardSoure.cpp
 * @author Leon Farchau (leon2225)
 * @brief 
 * @version 0.1
 * @date 03.01.2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/************************************************************
 *  INCLUDES
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "hardware/gpio.h"
#include "debug_pins.h"

// For SD Card
#define FF_FS_READONLY 1
#include "ff.h"
#include "diskio.h"
#include "hw_config.h"
#include "f_util.h"
#include "sd_card.h"
//
#include "diskio.h" /* Declarations of disk functions */


#include "specificRegisters.h"
#include "comInterface.h"

/************************************************************
 *  DEFINES
 ************************************************************/


/************************************************************
 * Type definitions
 * **********************************************************/
#define I2C_ADDR 0x28
#define SDA_PIN 26
#define SCL_PIN 27


/************************************************************
 * Variables
 * **********************************************************/
DeviceSpecificConfiguration_t* g_config;
/* SDIO Interface */
static sd_sdio_if_t sdio_if = {
    /*
    Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
    The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
        CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
        As of this writing, SDIO_CLK_PIN_D0_OFFSET is 18,
            which is -14 in mod32 arithmetic, so:
        CLK_gpio = D0_gpio -14.
        D1_gpio = D0_gpio + 1;
        D2_gpio = D0_gpio + 2;
        D3_gpio = D0_gpio + 3;
    */
    .CMD_gpio = 18,
    .D0_gpio = 19,
    .DMA_IRQ_num = DMA_IRQ_1,
    .use_exclusive_DMA_IRQ_handler = true,
    .baud_rate = 15 * 1000 * 1000  // 15 MHz
};

/* Hardware Configuration of the SD Card socket "object" */
static sd_card_t sd_card = {
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if
};

/************************************************************
 * Function prototypes
 * **********************************************************/
void setup();
void asyncLoop();
void dataCallback(void* data, uint32_t length);
void configCallback(DeviceSpecificConfiguration_t* config, DeviceSpecificConfiguration_t* oldConfig);
size_t sd_get_num();
sd_card_t *sd_get_by_num(size_t num);

/************************************************************
 * Functions
 * **********************************************************/

int main()
{
    setup();

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    FATFS fs;
    char* data = (char*)malloc(50000);
    UINT bytesToRead = 500;
    UINT bytes_read = 0;
    uint32_t lastOffset = 0;
    const char* const filename = "filename.txt";
    for (int i = 0; i < 100000; i++)
    {

        FRESULT fr = f_mount(&fs, "", 1);
        if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        FIL fil;
        fr = f_open(&fil, filename, FA_READ);
        if (FR_OK != fr && FR_EXIST != fr)
            panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        fr = f_lseek(&fil, lastOffset);
        lastOffset += bytesToRead;

        gpio_put(DEBUG_PIN3, 1);

        if (f_read(&fil, data, bytesToRead, &bytes_read) < 0) {
            printf("f_printf failed\n");
        }
        gpio_put(DEBUG_PIN3, 0);
        if(bytes_read != bytesToRead)
        {
            lastOffset = 0;
        }
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }
        f_unmount("");
    }
    

    while (true)
    {
        asyncLoop();
    }

    printf("done\n");

    return 0;
}

void setup()
{
    stdio_init_all();

    gpio_init(DEBUG_PIN1);
    gpio_init(DEBUG_PIN2);
    gpio_init(DEBUG_PIN3);
    gpio_set_dir(DEBUG_PIN1, GPIO_OUT);
    gpio_set_dir(DEBUG_PIN2, GPIO_OUT);
    gpio_set_dir(DEBUG_PIN3, GPIO_OUT);
    gpio_put(DEBUG_PIN1, 1);
    gpio_put(DEBUG_PIN2, 1);
    gpio_put(DEBUG_PIN3, 1);

    cominterfaceConfiguration config;
    config.g_i2c = i2c0;
    config.g_i2cAddr = I2C_ADDR;
    config.g_sdaPin = SDA_PIN;
    config.g_sclPin = SCL_PIN;
    config.HOut_Callback = dataCallback;
    config.UpdateConfig_Callback = configCallback;

    comInterfaceInit(&config);
}

void asyncLoop()
{
    static uint32_t lastSyncState = 0;
    volatile uint32_t syncState = 0;
    
    if(syncState != lastSyncState)
    {
        lastSyncState = syncState;

        if (syncState == 1)
        {
            gpio_put(DEBUG_PIN1, 1);
            uint8_t data[4] = {0};
            
            comInterfaceAddSample(data, 4);
            gpio_put(DEBUG_PIN1, 0);
        }
    }
}

void dataCallback(void* data, uint32_t length)
{
    // not implemented
}

void configCallback(DeviceSpecificConfiguration_t* config, DeviceSpecificConfiguration_t* oldConfig)
{
    g_config = config;
}

// SD Card callbacks
size_t sd_get_num() { return 1; }

sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num)
        return &sd_card;
    else
        return NULL;
}