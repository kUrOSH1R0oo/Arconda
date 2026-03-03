CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lcrypto -lz

SRCDIR = .
COREDIR = core
SOURCES = arconda.c \
          $(COREDIR)/util.c \
          $(COREDIR)/crypto.c \
          $(COREDIR)/filepack.c \
          $(COREDIR)/png.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = arconda
INSTALL_DIR = /usr/local/bin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@sudo cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@sudo chmod 755 $(INSTALL_DIR)/$(TARGET)
	@echo "[+] Arconda has been successfully installed. You can now use 'arconda' as a system command."

uninstall:
	@sudo rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "[+] Arconda has been successfully removed."

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(COREDIR)/*.o

.PHONY: all clean install uninstall