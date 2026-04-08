/**
 * --------------------------------------------------------------------------------------+
 * @brief       SSD1306 OLED Driver for Raspberry Pi (Linux)
 * --------------------------------------------------------------------------------------+
 * Modified for Linux/xwax project
 * --------------------------------------------------------------------------------------+
 */

#ifndef __SSD1306_H__
#define __SSD1306_H__

  // @includes
  #include <string.h>                     // memset function
  #include <stdint.h>                     // uint8_t, uint16_t definitions
  #include "font.h"                       // Tablica znaków (pamiętaj usunąć PROGMEM z font.h!)

  // Success / Error
  // ------------------------------------------------------------------------------------
  #define SSD1306_SUCCESS           0
  #define SSD1306_ERROR             1

  // Address definition
  // ------------------------------------------------------------------------------------
  #define SSD1306_ADDR              0x3C

  // Command definition
  // ------------------------------------------------------------------------------------
  #define SSD1306_COMMAND           0x80  // Continuation bit=1, D/C=0
  #define SSD1306_COMMAND_STREAM    0x00  // Continuation bit=0, D/C=0
  #define SSD1306_DATA              0xC0  // Continuation bit=1, D/C=1
  #define SSD1306_DATA_STREAM       0x40  // Continuation bit=0, D/C=1

  #define SSD1306_SET_MUX_RATIO     0xA8
  #define SSD1306_DISPLAY_OFFSET    0xD3
  #define SSD1306_DISPLAY_ON        0xAF
  #define SSD1306_DISPLAY_OFF       0xAE
  #define SSD1306_DIS_ENT_DISP_ON   0xA4
  #define SSD1306_DIS_IGNORE_RAM    0xA5
  #define SSD1306_DIS_NORMAL        0xA6
  #define SSD1306_DIS_INVERSE       0xA7
  #define SSD1306_DEACT_SCROLL      0x2E
  #define SSD1306_ACTIVE_SCROLL     0x2F
  #define SSD1306_SET_START_LINE    0x40
  #define SSD1306_MEMORY_ADDR_MODE  0x20
  #define SSD1306_SET_COLUMN_ADDR   0x21
  #define SSD1306_SET_PAGE_ADDR     0x22
  #define SSD1306_SEG_REMAP         0xA0
  #define SSD1306_SEG_REMAP_OP      0xA1
  #define SSD1306_COM_SCAN_DIR      0xC0
  #define SSD1306_COM_SCAN_DIR_OP   0xC8
  #define SSD1306_COM_PIN_CONF      0xDA
  #define SSD1306_SET_CONTRAST      0x81
  #define SSD1306_SET_OSC_FREQ      0xD5
  #define SSD1306_SET_CHAR_REG      0x8D
  #define SSD1306_SET_PRECHARGE     0xD9
  #define SSD1306_VCOM_DESELECT     0xDB
  #define SSD1306_NOP               0xE3
  #define SSD1306_RESET             0xE4

  // Clear Color
  // ------------------------------------------------------------------------------------
  #define CLEAR_COLOR               0x00

  // Init Status
  // ------------------------------------------------------------------------------------
  #define INIT_STATUS               0xFF

  // AREA definition (Dla 128x64)
  // ------------------------------------------------------------------------------------
  #define START_PAGE_ADDR           0
  #define END_PAGE_ADDR             7     
  #define START_COLUMN_ADDR         0
  #define END_COLUMN_ADDR           127
  #define RAM_X_END                 (END_COLUMN_ADDR + 1)
  #define RAM_Y_END                 (END_PAGE_ADDR + 1)

  #define CACHE_SIZE_MEM            ((1 + END_PAGE_ADDR) * (1 + END_COLUMN_ADDR))

  #define MAX_X                     END_COLUMN_ADDR
  #define MAX_Y                     ((END_PAGE_ADDR + 1) * 8)

  // @var set area - extern, aby nie było błędu "multiple definition"
  extern uint16_t _counter;
  
  /**
   * --------------------------------------------------------------------------------------------+
   * PUBLIC FUNCTIONS (Adaptacja do Linux/Raspberry Pi)
   * --------------------------------------------------------------------------------------------+
   */

  uint8_t SSD1306_Init (uint8_t);
  uint8_t SSD1306_Send_Command (uint8_t);
  void SSD1306_ClearScreen (void);
  uint8_t SSD1306_NormalScreen (uint8_t);
  uint8_t SSD1306_InverseScreen (uint8_t);
  uint8_t SSD1306_UpdateScreen (uint8_t);
  uint8_t SSD1306_UpdatePosition (void);
  void SSD1306_SetPosition (uint8_t, uint8_t);
  uint8_t SSD1306_DrawChar (char);
  void SSD1306_DrawString (char *);
  uint8_t SSD1306_DrawPixel (uint8_t, uint8_t);
  uint8_t SSD1306_DrawLine (uint8_t, uint8_t, uint8_t, uint8_t);

#endif