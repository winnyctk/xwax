#
# Makefile dla xwax z obsługą OLED (ArduiPi_OLED)
#

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
LIBEXECDIR = $(PREFIX)/libexec

# Kompilatory
CC = gcc
CXX = g++

# Flagi kompilacji
# Dodajemy obsługę SDL2, ALSA oraz ścieżkę do nagłówków ArduiPi
CFLAGS = -O3 -Wall `sdl2-config --cflags`
CXXFLAGS = -O3 -Wall -fpermissive

# Definicje systemowe
CPPFLAGS = -D_GNU_SOURCE -DEXECDIR=\"$(LIBEXECDIR)\" -DVERSION=\"1.9\"

# Biblioteki (Linker)
# -lArduiPi_OLED: Twoja nowa biblioteka
# -lstdc++: wymagane do linkowania kodu C++ z C
# -lunistring: wymagane przez xwax do obsługi znaków
LDLIBS = -lArduiPi_OLED -lstdc++ -lunistring -lm `sdl2-config --libs` -lpthread

# Opcjonalna obsługa ALSA
ifeq ($(ALSA),yes)
	CPPFLAGS += -DWITH_ALSA
	LDLIBS += -lasound
endif

# Lista plików obiektowych
OBJS = \
	controller.o \
	cues.o \
	deck.o \
	device.o \
	dummy.o \
	excrate.o \
	external.o \
	index.o \
	interface.o \
	library.o \
	listbox.o \
	lut.o \
	player.o \
	realtime.o \
	rig.o \
	selector.o \
	status.o \
	thread.o \
	timecoder.o \
	track.o \
	xwax.o \
	hw_ctrl.o

# Główny cel
all: xwax

# Reguła linkowania (Używamy CXX, żeby poprawnie połączyć C++ i C)
xwax: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

# Reguła dla standardowych plików .c
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# SPECJALNA REGUŁA DLA hw_ctrl.cpp (Wymusza g++)
# WAŻNE: Przed $(CXX) musi być znak TABULACJI
hw_ctrl.o: hw_ctrl.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f xwax $(OBJS) *.d

.PHONY: all clean