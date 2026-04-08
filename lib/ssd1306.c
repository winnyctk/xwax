/**
 * --------------------------------------------------------------------------------------+
 * @brief        SSD1306 OLED Driver for Raspberry Pi (Linux)
 * --------------------------------------------------------------------------------------+
 * Modified for Raspberry Pi / xwax project
 * --------------------------------------------------------------------------------------+
 */

#include "ssd1306.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// Globalny deskryptor pliku I2C dla Raspberry Pi
static int i2c_fd = -1;

// Rezygnujemy z PROGMEM i pgm_read_byte - na Linuxie to zwykła pamięć RAM
const uint8_t INIT_SSD1306[] = {
  19,                                                             // liczba komend
  SSD1306_DISPLAY_OFF, 0,                                         // 0xAE
  SSD1306_SET_MUX_RATIO, 1, 0x3F,                                 // 0xA8, 64MUX
  SSD1306_MEMORY_ADDR_MODE, 1, 0x00,                              // 0x20, Horizontal
  SSD1306_SET_START_LINE, 0,                                      // 0x40
  SSD1306_DISPLAY_OFFSET, 1, 0x00,                                // 0xD3
  SSD1306_SEG_REMAP_OP, 0,                                        // 0xA0
  SSD1306_COM_SCAN_DIR_OP, 0,                                     // 0xC0
  SSD1306_COM_PIN_CONF, 1, 0x12,                                  // 0xDA
  SSD1306_SET_CONTRAST, 1, 0x7F,                                  // 0x81
  SSD1306_DIS_ENT_DISP_ON, 0,                                     // 0xA4
  SSD1306_DIS_NORMAL, 0,                                          // 0xA6
  SSD1306_SET_OSC_FREQ, 1, 0x80,                                  // 0xD5
  SSD1306_SET_PRECHARGE, 1, 0xc2,                                 // 0xD9
  SSD1306_VCOM_DESELECT, 1, 0x20,                                 // 0xDB
  SSD1306_SET_CHAR_REG, 1, 0x14,                                  // 0x8D
  SSD1306_DEACT_SCROLL, 0,                                        // 0x2E
  SSD1306_SET_COLUMN_ADDR, 2, 0, 127,                             // 0x21
  SSD1306_SET_PAGE_ADDR, 2, 0, 7,                                 // 0x22
  SSD1306_DISPLAY_ON, 0                                           // 0xAF
};

static char cacheMemLcd[CACHE_SIZE_MEM];
static uint16_t _counter = 0;

/**
 * @brief   Inicjalizacja I2C na Raspberry Pi
 */
uint8_t SSD1306_Init(uint8_t address)
{
    const uint8_t *list = INIT_SSD1306;
    uint8_t commands = *list++;
    uint8_t arguments;

    // Otwarcie magistrali I2C
    if ((i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0) {
        return SSD1306_ERROR;
    }

    // Ustawienie adresu Slave
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) {
        return SSD1306_ERROR;
    }

    while (commands--) {
        uint8_t cmd = *list++;
        SSD1306_Send_Command(cmd);
        
        arguments = *list++;
        while (arguments--) {
            SSD1306_Send_Command(*list++);
        }
    }

    return SSD1306_SUCCESS;
}

/**
 * @brief   Wysyłanie komendy przez Linux I2C
 */
uint8_t SSD1306_Send_Command(uint8_t command)
{
    uint8_t buf[2];
    buf[0] = SSD1306_COMMAND; // 0x00
    buf[1] = command;
    if (write(i2c_fd, buf, 2) != 2) return SSD1306_ERROR;
    return SSD1306_SUCCESS;
}

/**
 * @brief   Aktualizacja ekranu (wysłanie całego bufora)
 */
uint8_t SSD1306_UpdateScreen(uint8_t address)
{
    // Na Linuxie wysyłamy strumień danych w jednej paczce (lub dzielonej)
    // SSD1306_DATA_STREAM = 0x40
    uint8_t buf[CACHE_SIZE_MEM + 1];
    buf[0] = SSD1306_DATA_STREAM;
    memcpy(&buf[1], cacheMemLcd, CACHE_SIZE_MEM);

    if (write(i2c_fd, buf, CACHE_SIZE_MEM + 1) != CACHE_SIZE_MEM + 1) {
        return SSD1306_ERROR;
    }

    return SSD1306_SUCCESS;
}

// Pozostałe funkcje operujące na buforze (cacheMemLcd) pozostają bez zmian, 
// bo nie dotykają sprzętu bezpośrednio.

void SSD1306_ClearScreen(void) {
    memset(cacheMemLcd, 0x00, CACHE_SIZE_MEM);
}

void SSD1306_SetPosition(uint8_t x, uint8_t y) {
    _counter = x + (y << 7);
}

uint8_t SSD1306_DrawChar(char character) {
    // Uwaga: Tutaj musisz mieć dostęp do tablicy FONTS z nagłówka
    // Oryginał używał pgm_read_byte, tutaj czytamy bezpośrednio:
    for (uint8_t i = 0; i < CHARS_COLS_LENGTH; i++) {
        cacheMemLcd[_counter++] = FONTS[character - 32][i];
    }
    _counter++;
    return SSD1306_SUCCESS;
}

void SSD1306_DrawString(char *str) {
    while (*str) {
        SSD1306_DrawChar(*str++);
    }
}

uint8_t SSD1306_DrawPixel(uint8_t x, uint8_t y) {
    if ((x > MAX_X) || (y > MAX_Y)) return SSD1306_ERROR;
    uint8_t page = y >> 3;
    uint8_t pixel = 1 << (y & 7);
    cacheMemLcd[x + (page << 7)] |= pixel;
    return SSD1306_SUCCESS;
}

// Funkcje kolorów i linii
uint8_t SSD1306_NormalScreen(uint8_t address) {
    return SSD1306_Send_Command(SSD1306_DIS_NORMAL);
}

uint8_t SSD1306_InverseScreen(uint8_t address) {
    return SSD1306_Send_Command(SSD1306_DIS_INVERSE);
}