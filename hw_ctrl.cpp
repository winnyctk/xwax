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

/**
 * WŁASNA FUNKCJA RYSOWANIA PIKSELA Z OBROTEM
 * Ponieważ biblioteka ma zakomentowany kod rotacji, 
 * robimy to tutaj.
 */
void drawPixelRotated(int16_t x, int16_t y, uint16_t color) {
    // Obrót o 180 stopni:
    // x = szerokość - x - 1
    // y = wysokość - y - 1
    display.drawPixel(127 - x, 63 - y, color);
}

/**
 * WŁASNA FUNKCJA RYSOWANIA LINII Z OBROTEM
 */
void drawLineRotated(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    display.drawLine(127 - x0, 63 - y0, 127 - x1, 63 - y1, color);
}

/**
 * WŁASNA FUNKCJA WYŚWIETLANIA TEKSTU Z OBROTEM
 * Biblioteka Adafruit_GFX używa drawPixel do wszystkiego.
 * My napiszemy prosty wrapper dla tekstu.
 */
void printRotated(int16_t x, int16_t y, const char* text) {
    // Musimy "ręcznie" wypisywać znaki, bo display.print używa standardowego drawPixel
    // Używamy wbudowanej czcionki 5x7 (jeden znak zajmuje 6x8 pikseli z odstępem)
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    int16_t currX = x;
    while (*text) {
        // Rysujemy pojedynczy znak 'ręcznie' z obrotem
        // drawChar to funkcja z Adafruit_GFX, która używa drawPixel()
        // Ale uwaga: ona też rysuje "prosto". 
        // Najskuteczniejsza metoda przy tej bibliotece to:
        display.setCursor(127 - currX - 5, 63 - y - 7);
        // Tu jest trik: wysyłamy komendy do sterownika TUŻ PRZED display()
        display.print(*text);
        text++;
        currX += 6; 
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
    
    // To usuwa logo Adafruit
    display.clearDisplay();
    display.display();

    int test_selection = 0;
    int counter = 0;

    while (1) {
        display.clearDisplay();

        // --- KLUCZ DO OBRÓCENIA EKRANU ---
        // Skoro biblioteka ma zakomentowany kod, wysyłamy komendy do chipu SSD1306
        // w każdej klatce pętli, zaraz po clearDisplay().
        display.sendCommand(0xA1); // Segment remap (Horizontal Flip)
        display.sendCommand(0xC8); // COM scan direction (Vertical Flip)

        // Teraz rysujemy już normalnie
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print((char*)"--- SYSTEM READY ---");
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