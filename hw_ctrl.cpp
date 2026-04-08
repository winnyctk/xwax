#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// 1. WCZYTUJEMY ARDUIPI
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// 2. CZYŚCIMY PSUJĄCE MAKRA (zanim wczytamy resztę systemu)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef swap
#undef swap
#endif

// 3. ROZWIĄZANIE PROBLEMU SŁOWA 'new'
// Przebiegle zamieniamy słowo 'new' na coś innego tylko na czas czytania nagłówków xwax
#define new _new_ptr

extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}

// Przywracamy 'new' dla C++
#undef new

// Globalny obiekt wyświetlacza
ArduiPi_OLED display;

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja ekranu
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        fprintf(stderr, "OLED init failed\n");
        return NULL;
    }

    display.begin();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.display();

    int oled_offset = 0;
    const int max_lines = 6;

    while (1) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print((char*)"--- BROWSER ---");
        display.drawLine(0, 10, 127, 10, WHITE);

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
            display.setCursor(0, 14 + (i * 8));

            if (item_idx == current_sel) display.print((char*)"> ");
            else display.print((char*)"  ");

            char buf[32];
            snprintf(buf, sizeof(buf), "%.20s", r->title);
            display.print(buf);
        }

        display.display();
        usleep(40000);
    }
    return NULL;
}

extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}