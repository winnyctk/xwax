#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* * 1. NAGŁÓWKI C++ (Musi być POZA extern "C")
 * Wczytujemy je najpierw, aby kompilator mógł użyć szablonów i klas.
 */
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

/* * 2. SEKCIJA C (Struktury xwax)
 * Owinięcie w extern "C" informuje linker, że te funkcje mają nazwy 
 * zgodne ze standardem C, co pozwala reszcie projektu xwax je widzieć.
 */
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}

// Globalna instancja wyświetlacza
ArduiPi_OLED display;

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    /*
     * Inicjalizacja OLED
     * 0 = OLED_ADAFRUIT_I2C_128x64 (najczęstszy model)
     */
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        fprintf(stderr, "hw_ctrl: Nie można zainicjować OLED\n");
        return NULL;
    }

    display.begin();
    display.clearDisplay();
    display.setTextSize(1);      // Rozmiar 1: klasyczna czcionka Adafruit 5x7 pikseli
    display.setTextColor(WHITE);
    display.display();

    int oled_offset = 0;
    const int max_lines = 6;     // Liczba utworów wyświetlanych pod linią

    while (1) {
        display.clearDisplay();

        // --- NAGŁÓWEK ---
        display.setCursor(0, 0);
        display.print((char*)"--- BROWSER ---");
        
        // Rysowanie linii pod nagłówkiem (y = 10)
        display.drawLine(0, 10, 127, 10, WHITE);

        // Pobranie danych z biblioteki xwax
        struct index *idx = hw->sel->view_index;
        int current_sel = listbox_current(&hw->sel->records);

        // Logika scrollowania listy
        if (current_sel >= 0) {
            if (current_sel < oled_offset) {
                oled_offset = current_sel;
            } else if (current_sel >= oled_offset + max_lines) {
                oled_offset = current_sel - max_lines + 1;
            }
        }

        // --- RYSOWANIE LISTY UTWORÓW ---
        for (int i = 0; i < max_lines; i++) {
            int item_idx = oled_offset + i;
            if (item_idx >= (int)idx->entries) break;

            struct record *r = idx->record[item_idx];
            
            // Pozycja Y: start od 14 (pod linią), odstęp 8 pikseli
            display.setCursor(0, 14 + (i * 8));

            if (item_idx == current_sel) {
                display.print((char*)"> ");
            } else {
                display.print((char*)"  ");
            }

            // Bezpieczne kopiowanie i wyświetlanie tytułu
            char buf[32];
            snprintf(buf, sizeof(buf), "%.20s", r->title);
            display.print(buf);
        }

        // Fizyczny update ekranu (wysłanie bufora przez I2C)
        display.display();
        
        // Czekamy ~40ms (ok. 25 klatek na sekundę)
        usleep(40000);
    }
    return NULL;
}

/* * Funkcja startowa wątku sterowania hardwarem.
 * Musi być extern "C", bo xwax.c jej szuka.
 */
extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}