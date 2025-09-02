# myWM Window Manager
# Modular source files for better organization

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lX11

SRCDIR = src
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/window.c $(SRCDIR)/events.c $(SRCDIR)/utils.c $(SRCDIR)/globals.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = mywm

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/mywm.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)
