#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// 1. Nagłówki ArduiPi / Adafruit
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Czyszczenie makr
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef swap
#undef swap
#endif

#define new _new_ptr
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}
#undef new

ArduiPi_OLED display;

// Dane testowe
const char* test_playlist[] = {
    "DAFT PUNK - ONE MORE TIME",
    "PRODIGY - FIRESTARTER",
    "KRAFTWERK - THE MODEL",
    "MOBY - GO",
    "JUSTICE - GENESIS",
    "UNDERWORLD - BORN SLIPPY"
};

static void* hw_thread_loop(void *arg) {
    // struct hw_state *hw = (struct hw_state*)arg; // Zakomentowane, by nie było warningu

    // Inicjalizacja ekranu
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        return NULL;
    }

    display.begin();
    
    // --- SPRZĘTOWY OBRÓT O 180 STOPNI (SSD1306 COMMANDS) ---
    // Te komendy odwracają mapowanie segmentów i skanowanie linii
    display.sendCommand(0xA1); // Segment remap (Horizontal flip)
    display.sendCommand(0xC8); // COM scan direction (Vertical flip)

    display.clearDisplay(); 
    display.display(); // Logo Adafruit zniknie natychmiast
    
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int test_selection = 0;
    int counter = 0;

    while (1) {
        display.clearDisplay();

        // Nagłówek
        display.setCursor(0, 0);
        display.print((char*)"--- ROTATED BROWSER ---");
        display.drawLine(0, 10, 127, 10, WHITE);

        // Symulacja ruchu
        if (++counter > 30) {
            test_selection = (test_selection + 1) % 6;
            counter = 0;
        }

        // Rysowanie testowej listy
        for (int i = 0; i < 6; i++) {
            display.setCursor(0, 14 + (i * 8));
            if (i == test_selection) display.print((char*)"> ");
            else display.print((char*)"  ");
            
            display.print((char*)test_playlist[i]);
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