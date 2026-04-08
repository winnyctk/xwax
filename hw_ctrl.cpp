#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Nagłówki ArduiPi_OLED
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Sekcja dla xwax (C) - to pozwala C++ widzieć struktury xwaxa
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}

// Inicjalizacja globalnego obiektu ekranu
ArduiPi_OLED display;

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja: 0 = OLED_ADAFRUIT_I2C_128x64
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        fprintf(stderr, "Błąd inicjalizacji OLED\n");
        return NULL;
    }

    display.begin();
    display.clearDisplay();
    display.setTextSize(1);      // Rozmiar 1 = czcionka 5x7 pikseli
    display.setTextColor(WHITE);
    display.display();

    int oled_offset = 0;
    const int max_lines = 6;     // Zostawiamy miejsce na nagłówek i linię separatora

    while (1) {
        display.clearDisplay();

        // --- NAGŁÓWEK ---
        display.setCursor(0, 0);
        display.print("--- BROWSER ---");
        
        // LINIA (Adafruit GFX)
        display.drawLine(0, 10, 127, 10, WHITE);

        struct index *idx = hw->sel->view_index;
        int current_sel = listbox_current(&hw->sel->records);

        if (current_sel >= 0) {
            if (current_sel < oled_offset) oled_offset = current_sel;
            else if (current_sel >= oled_offset + max_lines) oled_offset = current_sel - max_lines + 1;
        }

        // --- LISTA UTWORÓW ---
        for (int i = 0; i < max_lines; i++) {
            int item_idx = oled_offset + i;
            if (item_idx >= (int)idx->entries) break;

            struct record *r = idx->record[item_idx];
            
            // Każda linia ma 8 pikseli wysokości
            display.setCursor(0, 14 + (i * 8));

            if (item_idx == current_sel) {
                display.print("> ");
            } else {
                display.print("  ");
            }

            // Ograniczamy tytuł do szerokości ekranu
            char buf[24];
            snprintf(buf, sizeof(buf), "%.20s", r->title);
            display.print(buf);
        }

        display.display(); // Fizyczna aktualizacja ekranu
        usleep(40000);     // Odświeżanie ~25 FPS
    }
    return NULL;
}

// Funkcja wywoływana przez xwax przy starcie
extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}