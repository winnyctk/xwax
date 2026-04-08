#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// 1. Nagłówki ArduiPi / Adafruit
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Czyszczenie makr kolidujących z biblioteką standardową
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef swap
#undef swap
#endif

// Trik dla słowa kluczowego 'new' w nagłówkach C projektu xwax
#define new _new_ptr
extern "C" {
    #include "hw_ctrl.h"
    #include "selector.h"
    #include "library.h"
    #include "deck.h"
}
#undef new

// Globalny obiekt wyświetlacza
ArduiPi_OLED display;

// Dane testowe do playlisty
const char* test_playlist[] = {
    "DAFT PUNK - ONE MORE TIME",
    "PRODIGY - FIRESTARTER",
    "KRAFTWERK - THE MODEL",
    "MOBY - GO",
    "JUSTICE - GENESIS",
    "UNDERWORLD - BORN SLIPPY",
    "THE CHEMICAL BROTHERS - BLOCK"
};

static void* hw_thread_loop(void *arg) {
    // Inicjalizacja ekranu (I2C, model 128x64)
    if (!display.init(OLED_I2C_RESET, OLED_ADAFRUIT_I2C_128x64)) {
        fprintf(stderr, "hw_ctrl: Nie udalo sie zainicjowac ekranu!\n");
        return NULL;
    }

    display.begin();

    // --- KLUCZOWA POPRAWKA: OBRÓT O 180 STOPNI ---
    // Rzutujemy display na klasę Adafruit_GFX, aby wywołać publiczną metodę setRotation.
    // 0 = 0 stopni, 1 = 90, 2 = 180, 3 = 270.
    ((Adafruit_GFX*)&display)->setRotation(2);

    // Natychmiastowe czyszczenie, aby pozbyć się logo Adafruit
    display.clearDisplay();
    display.display();
    
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int test_selection = 0;
    int counter = 0;
    const int max_lines = 6;

    while (1) {
        display.clearDisplay();

        // Nagłówek (teraz po obrocie będzie poprawnie na górze)
        display.setCursor(0, 0);
        display.print((char*)"--- TEST PLAYLIST ---");
        
        // Linia oddzielająca nagłówek
        display.drawLine(0, 10, 127, 10, WHITE);

        // Symulacja poruszania się po liście co ok. 1.2 sekundy
        if (++counter > 30) {
            test_selection = (test_selection + 1) % 7;
            counter = 0;
        }

        // Rysowanie 6 utworów z naszej testowej tablicy
        for (int i = 0; i < max_lines; i++) {
            // Y zaczyna się od 14 (pod linią nagłówka), skok co 8 pikseli
            display.setCursor(0, 14 + (i * 8));

            if (i == test_selection) {
                display.print((char*)"> ");
            } else {
                display.print((char*)"  ");
            }

            // Wyświetlamy skrócony tytuł
            char buf[32];
            snprintf(buf, sizeof(buf), "%.18s", test_playlist[i]);
            display.print(buf);
        }

        // Wysłanie danych do fizycznego ekranu
        display.display();
        
        // Czekamy 40ms (ok. 25 klatek na sekundę)
        usleep(40000); 
    }
    return NULL;
}

// Funkcja wywoływana przez xwax przy starcie
extern "C" int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    // Tworzymy osobny wątek dla ekranu, żeby nie blokować dźwięku
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}