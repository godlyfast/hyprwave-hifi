CC = gcc
CFLAGS = `pkg-config --cflags gtk4 gtk4-layer-shell-0`
LIBS = `pkg-config --libs gtk4 gtk4-layer-shell-0`
TARGET = hyprwave
SRC = main.c layout.c paths.c

# Installation paths
PREFIX ?= $(HOME)/.local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/hyprwave

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Installing HyprWave to $(PREFIX)..."
	install -Dm755 $(TARGET) $(BINDIR)/$(TARGET)
	install -Dm644 style.css $(DATADIR)/style.css
	@mkdir -p $(DATADIR)/icons
	install -Dm644 icons/*.svg $(DATADIR)/icons/
	@echo "Installation complete!"
	@echo "Run 'hyprwave' to start"

uninstall:
	@echo "Uninstalling HyprWave..."
	rm -f $(BINDIR)/$(TARGET)
	rm -rf $(DATADIR)
	@echo "Uninstall complete!"

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install uninstall run
