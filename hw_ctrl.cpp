/* 1. NAJPIERW WSZYSTKIE NAGŁÓWKI (C++ i C) */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Nagłówki ekranu
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

/* FIX: Adafruit definiuje makro 'swap', które psuje standardowe biblioteki C++.
   Musimy je usunąć zanim kompilator pójdzie dalej. */
#ifdef swap
#undef swap
#endif

/* 2. TERAZ NAGŁÓWKI PROJEKTU XWAX */
/* Musimy je wczytać tutaj, ale poza blokiem extern "C", 
   żeby ich wewnętrzne include'y (jak math.h) nie wywalały błędów. */
#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"
#include "deck.h"

/* 3. DOPIERO TERAZ OTWIERAMY BLOK EXTERN "C" */
/* Deklarujemy tylko te funkcje, które linker musi widzieć jako C. */
extern "C" {
    int hw_ctrl_init(struct hw_state *hw);
    struct record* selector_current(struct selector *sel);
    // Jeśli kompilator będzie narzekał na brak innych funkcji, dopisz je tutaj.
}

ArduiPi_OLED display;

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
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

// Implementacja funkcji inicjującej
extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}