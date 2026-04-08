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
#include "lib/ssd1306.h"

/* Piny GPIO - dopasuj do swoich fizycznych połączeń */
#define BTN_UP    4
#define BTN_DOWN  17
#define BTN_LOAD  27
#define BTN_BACK  22

static volatile uint32_t *gpio_reg = NULL;

/* Inicjalizacja GPIO przez /dev/gpiomem */
static int init_gpiomem() {
    int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    gpio_reg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return (gpio_reg == MAP_FAILED) ? -1 : 0;
}

static int read_gpio(int pin) {
    if (!gpio_reg && init_gpiomem() < 0) return 0;
    // Wciśnięty przycisk (zwarty do GND) daje stan 0, więc zwracamy 1
    return ((gpio_reg[13] & (1 << pin)) == 0);
}

/* * Funkcja pomocnicza do rysowania pionowego (litera pod literą)
 * Każda litera zajmuje jedną "stronę" (page) o wysokości 8px.
 */
static void draw_vertical_string(uint8_t x, uint8_t start_page, const char *str) {
    uint8_t current_page = start_page;
    const char *p = str;
    
    while (*p && current_page <= END_PAGE_ADDR) {
        SSD1306_SetPosition(x, current_page);
        SSD1306_DrawChar(*p);
        p++;
        current_page++;
    }
}

static void* hw_thread_loop(void *arg) {
    struct hw_state *hw = (struct hw_state*)arg;

    // Inicjalizacja wyświetlacza na szynie I2C-1, adres 0x3C
    if (SSD1306_Init(SSD1306_ADDR) != SSD1306_SUCCESS) {
        fprintf(stderr, "Błąd inicjalizacji OLED!\n");
        return NULL;
    }

    int last_up = 0, last_down = 0, last_load = 0, last_back = 0;

    while (1) {
        // Odczyt stanów przycisków
        int b_up = read_gpio(BTN_UP);
        int b_down = read_gpio(BTN_DOWN);
        int b_load = read_gpio(BTN_LOAD);
        int b_back = read_gpio(BTN_BACK);

        // Logika selektora
        if (b_up && !last_up) selector_up(hw->sel);
        if (b_down && !last_down) selector_down(hw->sel);
        
        // Zmiana decka (BACK)
        if (b_back && !last_back) {
            hw->active_deck = (hw->active_deck + 1) % hw->num_decks;
        }
        
        // Ładowanie utworu (LOAD)
        if (b_load && !last_load) {
            struct record *r = selector_current(hw->sel);
            if (r) deck_load(&hw->decks[hw->active_deck], r);
        }

        last_up = b_up; last_down = b_down; last_load = b_load; last_back = b_back;

        // --- AKTUALIZACJA EKRANU ---
        SSD1306_ClearScreen();

        // 1. Numer Decka (na górze)
        char hdr[16];
        snprintf(hdr, sizeof(hdr), "D:%d", hw->active_deck + 1);
        SSD1306_SetPosition(0, 0);
        SSD1306_DrawString(hdr);

        // 2. Tytuł utworu (pionowo poniżej nagłówka)
        struct record *r = selector_current(hw->sel);
        if (r) {
            draw_vertical_string(0, 2, r->title);
        } else {
            SSD1306_SetPosition(0, 2);
            SSD1306_DrawString("EMPTY");
        }

        SSD1306_UpdateScreen(SSD1306_ADDR);
        
        usleep(50000); // ok. 20 klatek na sekundę
    }
    return NULL;
}

int hw_ctrl_init(struct hw_state *hw) {
    pthread_t thread;
    return pthread_create(&thread, NULL, hw_thread_loop, (void*)hw);
}