#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#include "hw_ctrl.h"
#include "selector.h"
#include "library.h"

// Dołączamy nową, potężną bibliotekę
#include <ssd1306_i2c.h>

/* --- USTAWIENIA SPRZĘTOWE --- */
#define BTN_UP    4
#define BTN_DOWN  17
#define BTN_LOAD  27
#define BTN_BACK  22

static volatile uint32_t *gpio_reg = NULL;

/* Inicjalizacja bezpośredniego dostępu do GPIO (szybki odczyt przycisków) */
static int init_gpiomem() {
    int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    gpio_reg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return (gpio_reg == MAP_FAILED) ? -1 : 0;
}

static int read_gpio(int pin) {
    if (!gpio_reg && init_gpiomem() < 0) return 0;
    // Zwarcie do masy daje stan niski, więc negujemy
    return ((gpio_reg[13] & (1 << pin)) == 0);
}

/* --- GŁÓWNY WĄTEK EKRANU --- */

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // 1. Inicjalizacja I2C i OLED (Dopasowane do ekranu 128x64)
    ssd1306_i2c_t *oled = ssd1306_i2c_open("/dev/i2c-1", 0x3C, 128, 64, NULL);
    if (!oled) {
        fprintf(stderr, "[OLED] Nie można otworzyć /dev/i2c-1 (Sprawdź uprawnienia lub przewody)\n");
        return NULL;
    }

    if (ssd1306_i2c_display_initialize(oled) < 0) {
        fprintf(stderr, "[OLED] Błąd inicjalizacji ekranu!\n");
        ssd1306_i2c_close(oled);
        return NULL;
    }

    // 2. Utworzenie bufora ramek
    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
    
    // 3. Opcje rysowania - Obrót o 90 stopni!
    ssd1306_graphics_options_t opts[1];
    opts[0].type = SSD1306_OPT_ROTATE_PIXEL;
    opts[0].value.rotation_degrees = 90; // Jeśli będzie "do góry nogami", zmień na 270
    
    int last_up = 0, last_down = 0, last_load = 0, last_back = 0;

    while (1) {
        // --- LOGIKA PRZYCISKÓW ---
        int b_up = read_gpio(BTN_UP);
        int b_down = read_gpio(BTN_DOWN);
        int b_load = read_gpio(BTN_LOAD);
        int b_back = read_gpio(BTN_BACK);

        if (b_up && !last_up) selector_up(hw->sel);
        if (b_down && !last_down) selector_down(hw->sel);
        if (b_back && !last_back) hw->active_deck = (hw->active_deck + 1) % hw->num_decks;
        
        if (b_load && !last_load) {
            struct record *r = selector_current(hw->sel);
            if (r) deck_load(&hw->decks[hw->active_deck], r);
        }

        last_up = b_up; last_down = b_down; last_load = b_load; last_back = b_back;

        // --- RENDEROWANIE INTERFEJSU ---
        ssd1306_framebuffer_clear(fbp);
        ssd1306_framebuffer_box_t bbox;

        // Zbieranie danych o odtwarzaczu
        char hdr[32];
        snprintf(hdr, sizeof(hdr), "DECK %d/%d", hw->active_deck + 1, hw->num_decks);

        /* Funkcja draw_text_extra:
         * fbp - bufor, text - tekst
         * x, y - współrzędne (ponieważ obracamy o 90st, oś Y to teraz dłuższy bok ekranu)
         * rozmiar - 10 (możesz dostosować)
         * typ czcionki - DEFAULT
         */
        
        // Nagłówek (np. na górze)
        ssd1306_framebuffer_draw_text_extra(fbp, hdr, 0, 15, 12, SSD1306_FONT_DEFAULT, 4, opts, 1, &bbox);

        // Informacje o utworze
        struct record *r = selector_current(hw->sel);
        if (r) {
            // Tytuł w rzędzie poniżej (y=40)
            ssd1306_framebuffer_draw_text_extra(fbp, r->title, 0, 40, 10, SSD1306_FONT_DEFAULT, 4, opts, 1, &bbox);
            
            // Jeśli masz Artystę
            if (r->artist) {
                ssd1306_framebuffer_draw_text_extra(fbp, r->artist, 0, 60, 10, SSD1306_FONT_DEFAULT, 4, opts, 1, &bbox);
            }
        } else {
            ssd1306_framebuffer_draw_text_extra(fbp, "Empty Crate", 0, 40, 10, SSD1306_FONT_DEFAULT, 4, opts, 1, &bbox);
        }

        // Wysłanie bufora do ekranu
        ssd1306_i2c_display_update(oled, fbp);
        
        usleep(40000); // ok. 25 FPS
    }

    // (Kod nigdy tu nie dotrze, wątek działa w nieskończoność, 
    // ale dobrą praktyką jest posiadanie sekcji czyszczącej).
    ssd1306_framebuffer_destroy(fbp);
    ssd1306_i2c_close(oled);
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}