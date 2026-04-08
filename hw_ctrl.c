#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"
#include "font8x8_latin.h"

/* --- KONFIGURACJA SPRZĘTOWA --- */
#define I2C_ADDR 0x3C
#define BTN_UP    4
#define BTN_DOWN  17
#define BTN_LOAD  27
#define BTN_BACK  22

static int i2c_fd = -1;
static uint8_t oled_buffer[1024];

/* Tablica dla polskich znaków (Latin Extended-A) */
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
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
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

static void oled_draw_char(int x, int y, int char_idx, int invert) {
    if (x >= 128 || y >= 8) return;
    const uint8_t *bitmap;

    if (char_idx <= 127) bitmap = (const uint8_t*)font8x8_basic[char_idx];
    else if (char_idx <= 159) bitmap = (const uint8_t*)font8x8_control[char_idx - 128];
    else if (char_idx <= 255) bitmap = (const uint8_t*)font8x8_ext_latin[char_idx - 160];
    else if (char_idx <= 271) bitmap = font8x8_pl[char_idx - 256];
    else return;

    for (int i = 0; i < 8; i++) {
        uint8_t col = bitmap[i];
        if (invert) col = ~col;
        oled_buffer[y * 128 + x + i] = col;
    }
}

/* --- DEKODER UTF-8 --- */

static int get_utf8_idx(const unsigned char **str) {
    unsigned char c1 = **str; (*str)++;
    if (c1 <= 127) return c1;
    if ((c1 & 0xE0) == 0xC0) {
        unsigned char c2 = **str; if (c2) (*str)++;
        if (c1 == 0xC2) return c2;
        if (c1 == 0xC3) return c2 + 64;
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

static void oled_draw_string(int x, int y, const char *str, int invert) {
    const unsigned char *p = (const unsigned char *)str;
    while (*p && x < 120) {
        oled_draw_char(x, y, get_utf8_idx(&p), invert);
        x += 8;
    }
}

/* --- GPIO --- */

static int read_gpio(int pin) {
    char path[64], val[3];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    if (read(fd, val, 3) < 1) { close(fd); return 0; }
    close(fd);
    return (val[0] == '0');
}

/* --- WĄTEK GŁÓWNY --- */

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
            struct record *r = selector_get_selected(hw->sel);
            if (r) deck_load(&hw->decks[hw->active_deck], r);
        }

        last_up = b_up; last_down = b_down; last_load = b_load; last_back = b_back;

        memset(oled_buffer, 0, 1024);
        
        char hdr[32];
        snprintf(hdr, 32, "DECK %d/%d", hw->active_deck + 1, hw->num_decks);
        oled_draw_string(0, 0, hdr, 0);

        int selected = selector_get_selected_index(hw->sel);
        int n_entries = selector_get_nb_entries(hw->sel);
        int start = selected - 2;
        if (start < 0) start = 0;

        for (int i = 0; i < 5; i++) {
            int curr = start + i;
            if (curr >= n_entries) break;
            struct record *r = selector_get_entry(hw->sel, curr);
            if (r) oled_draw_string(0, i + 2, r->title, (curr == selected));
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