#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* * 1. NAGŁÓWKI ARDUIPI / ADAFRUIT 
 * Muszą być na samym początku, przed jakimkolwiek kodem C++.
 */
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

/* * FIX: Adafruit definiuje makro 'swap', które koliduje z biblioteką 
 * standardową C++ (używaną przez deck.h). Usuwamy je tutaj.
 */
#ifdef swap
#undef swap
#endif

/* * 2. NAGŁÓWKI XWAX (C) 
 * Zawijamy je w extern "C", aby g++ nie zmieniał nazw funkcji 
 * i pozwolił linkerowi połączyć je z resztą projektu w C.
 */
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}

// Globalny obiekt wyświetlacza
ArduiPi_OLED display;

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja ekranu (0 = SSD1306 128x64 I2C)
    // Jeśli używasz innego modelu, sprawdź parametry w demo ArduiPi
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        fprintf(stderr, "hw_ctrl: Nie można zainicjować OLED (check address 0x3C?)\n");
        return NULL;
    }

    display.begin();
    display.clearDisplay();
    display.setTextSize(1);      // Mała czcionka (5x7 pikseli)
    display.setTextColor(WHITE);
    display.display();

    int oled_offset = 0;
    const int max_lines = 6;     // Liczba linii tekstu mieszczących się pod nagłówkiem

    while (1) {
        display.clearDisplay();

        // --- RYSOWANIE NAGŁÓWKA ---
        display.setCursor(0, 0);
        display.print((char*)"--- BROWSER ---");
        
        // Rysowanie linii poziomej pod nagłówkiem (y = 10)
        display.drawLine(0, 10, 127, 10, WHITE);

        // Pobranie danych o aktualnym widoku z xwax
        struct index *idx = hw->sel->view_index;
        int current_sel = listbox_current(&hw->sel->records);

        // Logika przewijania (scroll)
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
            
            // Start od y=14, każda linia ma 8 pikseli wysokości
            display.setCursor(0, 14 + (i * 8));

            if (item_idx == current_sel) {
                display.print((char*)"> ");
            } else {
                display.print((char*)"  ");
            }

            // Ograniczenie długości tekstu do ok. 20 znaków
            char buf[32];
            snprintf(buf, sizeof(buf), "%.20s", r->title);
            display.print(buf);
        }

        // Aktualizacja fizyczna wyświetlacza
        display.display();
        
        // Ok. 25 klatek na sekundę
        usleep(40000);
    }
    return NULL;
}

/* * Funkcja wywoływana przez xwax.c podczas startu programu.
 * Musi być extern "C".
 */
extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}