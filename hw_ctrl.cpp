#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

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

// Funkcja do programowego obracania bufora o 180 stopni
void rotateBuffer180() {
    uint8_t *buffer = display.getBuffer();
    // SSD1306 ma 1024 bajty bufora (128x64 / 8)
    int bufferSize = 1024; 
    
    // Obracamy bajty i bity "w miejscu"
    for (int i = 0; i < bufferSize / 2; i++) {
        uint8_t a = buffer[i];
        uint8_t b = buffer[bufferSize - 1 - i];

        // Odwracanie bitów w bajcie (bit reversal)
        auto reverseBits = [](uint8_t x) {
            x = ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
            x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
            x = ((x & 0xF0) >> 4) | ((x & 0x0F) << 4);
            return x;
        };

        buffer[i] = reverseBits(b);
        buffer[bufferSize - 1 - i] = reverseBits(a);
    }
}

const char* test_playlist[] = {
    "01. DAFT PUNK",
    "02. PRODIGY",
    "03. KRAFTWERK",
    "04. MOBY",
    "05. JUSTICE",
    "06. UNDERWORLD"
};

static void* hw_thread_loop(void *arg) {
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) return NULL;

    display.begin();
    display.clearDisplay();
    display.display();
    
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int test_selection = 0;
    int counter = 0;

    while (1) {
        display.clearDisplay();

        // Rysujemy normalnie (jakby ekran był prosto)
        display.setCursor(0, 0);
        display.print((char*)"--- SW ROTATED ---");
        display.drawLine(0, 10, 127, 10, WHITE);

        if (++counter > 30) {
            test_selection = (test_selection + 1) % 6;
            counter = 0;
        }

        for (int i = 0; i < 6; i++) {
            display.setCursor(0, 14 + (i * 8));
            if (i == test_selection) display.print((char*)"> ");
            else display.print((char*)"  ");
            display.print((char*)test_playlist[i]);
        }

        // --- KLUCZ: OBRACAMY CAŁY BUFOR W PAMIĘCI ---
        rotateBuffer180();

        // Teraz wysyłamy już obrócony bufor
        display.display();
        usleep(40000); 
    }
    return NULL;
}

extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}