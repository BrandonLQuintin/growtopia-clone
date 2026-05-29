CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 $(shell pkg-config --cflags sdl2) -D_GNU_SOURCE=1 -D_REENTRANT
LDFLAGS = $(shell pkg-config --libs sdl2) -lGL -lm

SRCDIR = src
OBJDIR = build

SOURCES = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/**/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET = growtopia

all: dirs $(TARGET)

dirs:
	mkdir -p $(OBJDIR)/engine $(OBJDIR)/world $(OBJDIR)/game $(OBJDIR)/data

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run dirs
