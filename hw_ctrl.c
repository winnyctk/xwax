#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ssd1306_i2c.h>
#include "hw_ctrl.h"

static void* hw_thread_loop(void *arg) {
    const char *filename = "/dev/i2c-1";
    
    // Inicjalizacja 128x64 zgodnie z Twoim sprzętem
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0x3c, 128, 64, NULL);
    if (!oled) return NULL;

    if (ssd1306_i2c_display_initialize(oled) < 0) {
        ssd1306_i2c_close(oled);
        return NULL;
    }

    // Tworzenie bufora ramek
    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
    ssd1306_framebuffer_box_t bbox;

    while (1) {
        // Czyścimy bufor przed każdym rysowaniem
        ssd1306_framebuffer_clear(fbp);

        // Rysujemy 4 rzędy napisu TEST
        // Parametry: (bufor, tekst, x, y, rozmiar, czcionka, wcięcie, bbox)
        // Zwiększamy Y o 16 pikseli dla każdego rzędu
        ssd1306_framebuffer_draw_text(fbp, "TEST ROW 1", 0, 0,  12, SSD1306_FONT_DEFAULT, 4, &bbox);
        ssd1306_framebuffer_draw_text(fbp, "TEST ROW 2", 0, 16, 12, SSD1306_FONT_DEFAULT, 4, &bbox);
        ssd1306_framebuffer_draw_text(fbp, "TEST ROW 3", 0, 32, 12, SSD1306_FONT_DEFAULT, 4, &bbox);
        ssd1306_framebuffer_draw_text(fbp, "TEST ROW 4", 0, 48, 12, SSD1306_FONT_DEFAULT, 4, &bbox);

        // Wysłanie danych do fizycznego wyświetlacza
        ssd1306_i2c_display_update(oled, fbp);

        // Odświeżanie co 0.5 sekundy (dla testu)
        usleep(500000); 
    }

    // Sprzątanie (teoretycznie, pętla jest nieskończona)
    ssd1306_framebuffer_destroy(fbp);
    ssd1306_i2c_close(oled);
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    // Tworzymy wątek dla obsługi ekranu
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}