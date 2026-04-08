#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#include "Wrapper.h" // Kluczowy nagłówek dla C

#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja (Wrapper używa globalnej instancji)
    // 0 to OLED_ADAFRUIT_I2C_128x64 wg specyfikacji wrappera
    if (display_init(0) < 0) return NULL;

    display_begin();
    display_clearDisplay();
    display_setTextSize(1);
    display.setTextColor(WHITE);

    const int max_lines = 6;
    int oled_offset = 0;

    while (1) {
        display_clearDisplay();

        // Rysowanie tekstu w C
        display_setCursor(0, 0);
        display_puts("--- BROWSER (C) ---");

        // RYSOWANIE LINII (Funkcja Wrappera)
        display_drawLine(0, 10, 127, 10, WHITE);

        struct index *idx = hw->sel->view_index;
        int current_sel = listbox_current(&hw->sel->records);

        if (current_sel >= 0) {
            if (current_sel < oled_offset) oled_offset = current_sel;
            else if (current_sel >= oled_offset + max_lines) oled_offset = current_sel - max_lines + 1;
        }

        for (int i = 0; i < max_lines; i++) {
            int item_idx = oled_offset + i;
            if (item_idx >= (int)idx->entries) break;

            struct record *r = idx->record[item_idx];
            display_setCursor(0, 14 + (i * 8));

            if (item_idx == current_sel) display_puts("> ");
            else display_puts("  ");

            char buf[22];
            snprintf(buf, sizeof(buf), "%.20s", r->title);
            display_puts(buf);
        }

        display_display();
        usleep(30000);
    }
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}