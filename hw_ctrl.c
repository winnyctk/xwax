#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"
#include <ssd1306_i2c.h>

/* --- KONFIGURACJA PINÓW --- */
#define BTN_UP    4
#define BTN_DOWN  17
#define BTN_LOAD  27
#define BTN_BACK  22

static volatile uint32_t *gpio_reg = NULL;

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

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja ekranu 128x64
    ssd1306_i2c_t *oled = ssd1306_i2c_open("/dev/i2c-1", 0x3c, 128, 64, NULL);
    if (!oled) return NULL;

    if (ssd1306_i2c_display_initialize(oled) < 0) {
        ssd1306_i2c_close(oled);
        return NULL;
    }

    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
    ssd1306_framebuffer_box_t bbox;

    int last_up = 0, last_down = 0, last_load = 0, last_back = 0;
    int oled_offset = 0;
    const int max_lines = 4; // 4 linie utworów + nagłówek

    while (1) {
        // Obsługa przycisków
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

        // --- RYSOWANIE ---
        ssd1306_framebuffer_clear(fbp);

        struct index *idx = hw->sel->view_index;
        int current_sel = listbox_current(&hw->sel->records);

        // NAGŁÓWEK: Y=12 (baseline dla czcionki 12)
        ssd1306_framebuffer_draw_text(fbp, "--- BROWSER ---", 0, 12, 12, SSD1306_FONT_DEFAULT, 4, &bbox);

        // Logika okna listy (scroll offset)
        if (current_sel >= 0) {
            if (current_sel < oled_offset) oled_offset = current_sel;
            else if (current_sel >= oled_offset + max_lines) oled_offset = current_sel - max_lines + 1;
        }

        // LISTA UTWORÓW
        for (int i = 0; i < max_lines; i++) {
            int item_idx = oled_offset + i;
            if (item_idx >= idx->entries) break;

            struct record *r = idx->record[item_idx];
            char line_buf[32];

            if (item_idx == current_sel) snprintf(line_buf, sizeof(line_buf), ">%.20s", r->title);
            else snprintf(line_buf, sizeof(line_buf), " %.20s", r->title);

            // Obliczanie Y (baseline):
            // Nagłówek kończy się na Y=12. 
            // Pierwszy utwór: Y=26 (12 + 14)
            // Drugi utwór: Y=40 (26 + 14) itd.
            int y_pos = 26 + (i * 13); 

            ssd1306_framebuffer_draw_text(fbp, line_buf, 0, y_pos, 10, SSD1306_FONT_DEFAULT, 4, &bbox);
        }

        ssd1306_i2c_display_update(oled, fbp);
        usleep(40000); 
    }

    ssd1306_framebuffer_destroy(fbp);
    ssd1306_i2c_close(oled);
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}