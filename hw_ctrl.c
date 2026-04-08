#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/mman.h>

#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"
#include "font8x8_latin.h"

/* --- KONFIGURACJA --- */
#define I2C_ADDR 0x3C
#define BTN_UP    4
#define BTN_DOWN  17
#define BTN_LOAD  27
#define BTN_BACK  22

static int i2c_fd = -1;
static uint8_t oled_buffer[1024];
static volatile uint32_t *gpio_reg = NULL;

/* Tablica dla polskich znaków */
static const uint8_t font8x8_pl[16][8] = {
    {0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x06, 0x0C}, // ą
    {0x0C, 0x18, 0x3C, 0x60, 0x60, 0x66, 0x3C, 0x00}, // ć
    {0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x06, 0x0C}, // ę
    {0x60, 0x60, 0x60, 0x64, 0x68, 0x60, 0x3C, 0x00}, // ł
    {0x0C, 0x18, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, // ń
    {0x0C, 0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}, // ś
    {0x0C, 0x18, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // ź
    {0x00, 0x18, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // ż
    {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x0C}, // Ą
    {0x0C, 0x18, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}, // Ć
    {0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x06, 0x0C}, // Ę
    {0x60, 0x60, 0x64, 0x68, 0x60, 0x60, 0x7E, 0x00}, // Ł
    {0x0C, 0x18, 0x66, 0x76, 0x7E, 0x6E, 0x66, 0x00}, // Ń
    {0x0C, 0x18, 0x3C, 0x60, 0x3C, 0x06, 0x3C, 0x00}, // Ś
    {0x0C, 0x18, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // Ź
    {0x00, 0x18, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}  // Ż
};

/* --- OBSŁUGA SSD1306 --- */

static void oled_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    if (i2c_fd >= 0) write(i2c_fd, buf, 2);
}

static int oled_init(const char *dev) {
    i2c_fd = open(dev, O_RDWR);
    if (i2c_fd < 0) return -1;
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) return -1;

    uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 
        0xA1, // Segment re-map
        0xC8, // COM scan direction
        0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (size_t i = 0; i < sizeof(init_cmds); i++) oled_send_cmd(init_cmds[i]);
    return 0;
}

static void oled_send_buffer() {
    uint8_t buf[129];
    buf[0] = 0x40;
    for (int page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page);
        oled_send_cmd(0x00);
        oled_send_cmd(0x10);
        for (int i = 0; i < 128; i++) buf[i+1] = oled_buffer[page*128 + i];
        write(i2c_fd, buf, 129);
    }
}

/**
 * oled_draw_char z obrotem o 90 stopni
 * line: Linia tekstu (0-15) - biega wzdłuż dłuższego boku
 * y_offset: Przesunięcie od boku (0-56)
 */
static void oled_draw_char(int line, int y_offset, int char_idx, int invert) {
    if (line >= 16 || y_offset >= 64) return;
    const uint8_t *bitmap;

    if (char_idx <= 127) bitmap = (const uint8_t*)font8x8_basic[char_idx];
    else if (char_idx <= 159) bitmap = (const uint8_t*)font8x8_control[char_idx - 128];
    else if (char_idx <= 255) bitmap = (const uint8_t*)font8x8_ext_latin[char_idx - 160];
    else if (char_idx <= 271) bitmap = font8x8_pl[char_idx - 256];
    else return;

    for (int i = 0; i < 8; i++) {       // i = kolumna w fontcie
        uint8_t byte = bitmap[i];
        for (int j = 0; j < 8; j++) {   // j = bit w bajcie
            // Matematyczny obrót: 
            // Nowe X (0-127) to numer linii * 8 + bit j
            // Nowe Y (0-63) to y_offset + kolumna i
            int target_x = line * 8 + j;
            int target_y = y_offset + i;

            int pixel = (byte >> j) & 0x01;
            if (invert) pixel = !pixel;

            if (pixel) {
                oled_buffer[(target_y / 8) * 128 + target_x] |= (1 << (target_y % 8));
            } else {
                oled_buffer[(target_y / 8) * 128 + target_x] &= ~(1 << (target_y % 8));
            }
        }
    }
}

/* --- DEKODER UTF-8 --- */

static int get_utf8_idx(const unsigned char **str) {
    unsigned char c1 = **str; (*str)++;
    if (c1 <= 127) return c1;
    if ((c1 & 0xE0) == 0xC0) {
        unsigned char c2 = **str; if (c2) (*str)++;
        if (c1 == 0xC4) {
            if (c2 == 0x85) return 256; if (c2 == 0x87) return 257; if (c2 == 0x99) return 258;
            if (c2 == 0x84) return 264; if (c2 == 0x86) return 265; if (c2 == 0x98) return 266;
        }
        if (c1 == 0xC5) {
            if (c2 == 0x82) return 259; if (c2 == 0x84) return 260; if (c2 == 0x9B) return 261;
            if (c2 == 0xBA) return 262; if (c2 == 0xBC) return 263; if (c2 == 0x81) return 267;
            if (c2 == 0x83) return 268; if (c2 == 0x9A) return 269; if (c2 == 0xB9) return 270;
            if (c2 == 0xBB) return 271;
        }
    }
    return '?';
}

static void oled_draw_string(int line, int y_offset, const char *str, int invert) {
    const unsigned char *p = (const unsigned char *)str;
    int current_line = line;
    while (*p && current_line < 16) {
        oled_draw_char(current_line, y_offset, get_utf8_idx(&p), invert);
        current_line++;
    }
}

/* --- GPIO (ODCZYT PAMIĘCI) --- */

static int init_gpiomem() {
    int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    gpio_reg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return (gpio_reg == MAP_FAILED) ? -1 : 0;
}

static int read_gpio(int pin) {
    if (!gpio_reg && init_gpiomem() < 0) return 0;
    return ((gpio_reg[13] & (1 << pin)) == 0);
}

/* --- PĘTLA GŁÓWNA --- */

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;
    oled_init("/dev/i2c-1");

    int last_up = 0, last_down = 0, last_load = 0, last_back = 0;

    while (1) {
        int b_up = read_gpio(BTN_UP);
        int b_down = read_gpio(BTN_DOWN);
        int b_load = read_gpio(BTN_LOAD);
        int b_back = read_gpio(BTN_BACK);

        if (b_up && !last_up) selector_up(hw->sel);
        if (b_down && !last_down) selector_down(hw->sel);
        if (b_back && !last_back) hw->active_deck = (hw->active_deck + 1) % hw->num_decks;
        
        if (b_load && !last_load) {
            struct record *r = selector_current(hw->sel);
            if (r) deck_load(&hw->decks[hw->active_deck], r);
        }

        last_up = b_up; last_down = b_down; last_load = b_load; last_back = b_back;

        memset(oled_buffer, 0, 1024);
        
        char hdr[32];
        snprintf(hdr, 32, "DECK %d/%d", hw->active_deck + 1, hw->num_decks);
        
        /* Rysowanie w pionie: line=0 (sama góra), y_offset=0 (od lewej) */
        oled_draw_string(0, 0, hdr, 0);

        struct record *r = selector_current(hw->sel);
        if (r) {
             /* Tytuł w liniach 2-15, lekko odsunięty od góry */
             oled_draw_string(2, 0, r->title, 1); 
             if (r->artist) {
                 oled_draw_string(4, 0, r->artist, 0);
             }
        }

        oled_send_buffer();
        usleep(40000); 
    }
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}