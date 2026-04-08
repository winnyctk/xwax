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

const char* test_playlist[] = {
    "DAFT PUNK - ONE MORE TIME",
    "PRODIGY - FIRESTARTER",
    "KRAFTWERK - THE MODEL",
    "MOBY - GO",
    "JUSTICE - GENESIS",
    "UNDERWORLD - BORN SLIPPY"
};

static void* hw_thread_loop(void *arg) {
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        return NULL;
    }

    display.begin();

    // --- RĘCZNE ODRÓCENIE MATRYCY (LOW-LEVEL) ---
    // Skoro biblioteka nie ma setRotation, wymuszamy to komendami na szynę I2C
    // 0xA0/0xA1 = Mapowanie segmentów (X)
    // 0xC0/0xC8 = Kierunek skanowania COM (Y)
    display.sendCommand(0xA1); // Odwróć X
    display.sendCommand(0xC8); // Odwróć Y

    display.clearDisplay();
    display.display();
    
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int test_selection = 0;
    int counter = 0;

    while (1) {
        display.clearDisplay();

        display.setCursor(0, 0);
        display.print((char*)"--- TEST PLAYLIST ---");
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

        display.display();
        usleep(40000); 
    }
    return NULL;
}

extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}