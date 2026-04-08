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

// Funkcja obracająca obraz poprzez przerysowanie pikseli
void softwareRotation180() {
    // Tworzymy tymczasową kopię stanów pikseli (128x64 bitów = 1024 bajty)
    uint8_t temp[128][64]; 

    // 1. Zczytujemy cały ekran do tablicy
    for (int16_t x = 0; x < 128; x++) {
        for (int16_t y = 0; y < 64; y++) {
            temp[x][y] = display.getPixel(x, y);
        }
    }

    // 2. Czyścimy bufor
    display.clearDisplay();

    // 3. Rysujemy piksele odwrócone (127-x, 63-y)
    for (int16_t x = 0; x < 128; x++) {
        for (int16_t y = 0; y < 64; y++) {
            if (temp[x][y]) {
                display.drawPixel(127 - x, 63 - y, WHITE);
            }
        }
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

        // Rysujemy wszystko "normalnie"
        display.setCursor(0, 0);
        display.print((char*)"--- SW ROTATE V2 ---");
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

        // --- MAGIA: OBRACAMY PRZEZ KOPIOWANIE PIKSELI ---
        softwareRotation180();

        display.display();
        usleep(40000); 
    }
    return NULL;
}

extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}