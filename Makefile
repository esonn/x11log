# (c) Sonnleitner Erik <es@delta-xi.net>
# Makefile for x11log

DESTDIR = /
CC = gcc
CFLAGS = -ggdb -Wall -fno-builtin-log
INCLUDES = -I/usr/lib/X11R6/include -I/usr/include/curl
DEFINES = -D_HAVE_CURL
LIBS = -L/usr/X11R6/lib
EXTRA_LDFLAGS = -lX11 -lcurl
LDFLAGS = -lX11 -lcurl
TARGET = x11log
MANPAGE_POD = manpage.pod
POD2MAN = /usr/bin/pod2man
MANDIR = /usr/share/man/man1

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $(TARGET).o $(LIBS) $(EXTRA_LDFLAGS) $(INCLUDES) $(DEFINES)
	$(POD2MAN) --center "Utils" $(MANPAGE_POD) > $(TARGET).1

$(TARGET).o: $(TARGET).c
	$(CC) $(INCLUDES) $(DEFINES) $(CFLAGS) -c $(TARGET).c

clean:
	rm -f *.o 
	rm -f $(TARGET)

install:
	mkdir -p $(DESTDIR)/usr/bin/
	install -m 0644 $(TARGET) $(DESTDIR)/usr/bin/
	install -m 0644 $(TARGET).1 $(MANDIR)/$(TARGET).1

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(TARGET)
	rm -f $(MANDIR)/$(TARGET).1
