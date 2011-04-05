# Sonnleitner Erik
# Makefile for x11log, an X11 Keylogger
# #
#
DESTDIR = .
CC = gcc
CFLAGS = -I/usr/lib/X11R6/include
LIBS = -L/usr/X11R6/lib
LDFLAGS = -lX11
TARGET = x11log

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $(TARGET).o $(LIBS) $(LDFLAGS)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $(TARGET).c

clean:
	rm -f *.o 
	rm -f $(TARGET)
