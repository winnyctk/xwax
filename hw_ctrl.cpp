#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// 1. Nagłówki ArduiPi / Adafruit
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Czyścimy makra kolidujące z C++
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef swap
#undef swap
#endif

// Trik dla słowa kluczowego 'new' w nagłówkach xwax
#define new _new_ptr
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}
#undef new

ArduiPi_OLED display;

// --- DANE TESTOWE ---
const char* test_playlist[] = {
    "01. Daft Punk - One More Time",
    "02. The Chemical Brothers - Block",
    "03. Fatboy Slim - Right Here",
    "04. Prodigy - Firestarter",
    "05. Kraftwerk - The Model",
    "06. Aphex Twin - Windowlicker",
    "07. Moby - Go",
    "08. Underworld - Born Slippy",
    "09. Gorillaz - Clint Eastwood",
    "10. Justice - Genesis"
};
int test_count = 10;
int test_selection = 0; // Symulacja wybranego utworu

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja ekranu (0 = SSD1306 128x64 I2C)
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        return NULL;
    }

    display.begin();
    
    // --- POPRAWKA: USUWANIE LOGO ADAFRUIT ---
    display.clearDisplay(); 
    display.display();      // Wysłanie pustego bufora natychmiast po starcie
    
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int oled_offset = 0;
    const int max_lines = 6;

    while (1) {
        display.clearDisplay();

        // Nagłówek
        display.setCursor(0, 0);
        display.print((char*)"--- TEST MODE ---");
        display.drawLine(0, 10, 127, 10, WHITE);

        // --- LOGIKA PRZEWIJANIA (TESTOWA) ---
        // Tutaj symulujemy, że co 2 sekundy wybór schodzi niżej
        // W prawdziwym xwax to będzie reagować na Twoją myszkę/kontroler
        static int counter = 0;
        if (++counter > 50) { // Co ok. 2 sekundy (50 * 40ms)
            test_selection = (test_selection + 1) % test_count;
            counter = 0;
        }

        if (test_selection < oled_offset) {
            oled_offset = test_selection;
        } else if (test_selection >= oled_offset + max_lines) {
            oled_offset = test_selection - max_lines + 1;
        }

        // --- RYSOWANIE LISTY ---
        for (int i = 0; i < max_lines; i++) {
            int item_idx = oled_offset + i;
            if (item_idx >= test_count) break;

            display.setCursor(0, 14 + (i * 8));

            if (item_idx == test_selection) {
                display.print((char*)"> ");
            } else {
                display.print((char*)"  ");
            }

            char buf[32];
            snprintf(buf, sizeof(buf), "%.20s", test_playlist[item_idx]);
            display.print(buf);
        }

        display.display();
        usleep(40000); // 25 FPS
    }
    return NULL;
}

extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}