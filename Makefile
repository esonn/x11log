# (c) Sonnleitner Erik <es@delta-xi.net>
# Makefile for x11log, updated for clean build on Arch

DESTDIR ?= .
CC = gcc
#CFLAGS = -ggdb -fPIE -m32 -Wall -fno-builtin-log -Wno-multichar -std=c99 -Wno-deprecated-declarations
#CFLAGS = -ggdb -fPIE -Wall -std=c99 -fno-builtin-log -Wall -Wno-multichar -Wno-missing-braces -Wno-deprecated-declarations
CFLAGS = -fPIE -Wall -std=c99 -fno-builtin-log -Wall -Wno-multichar -Wno-missing-braces -Wno-deprecated-declarations
# -fno-builtin-log:             we have our own implementation of log()
# -Wno-multichar:               we use multi-chars in the source (sorry for that)
# -Wno-depricated-declarations: we use XKeycodeToKeysym from Xlib.h
# -Wno-missing-braces:          see GCC bug 53119

INCLUDES ?= -I/usr/lib/X11R6/include -I/usr/include/curl
DEFINES = -D_HAVE_CURL -D_UNICODE -D_GNU_SOURCE
LIBS ?= -L/usr/X11R6/lib
EXTRA_LDFLAGS ?= -lX11 -lcurl
LDFLAGS ?= -lX11 -lcurl
TARGET ?= x11log
MANPAGE_POD ?= manpage.pod
POD2MAN ?= /usr/bin/pod2man
MANDIR ?= /usr/share/man/man1

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
	mkdir -p $(DESTDIR)/usr/share/man/man1/
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	install -m 0644 $(TARGET).1 $(DESTDIR)/usr/share/man/man1/$(TARGET).1

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(TARGET)
	rm -f $(DESTDIR)/usr/share/man/man1/$(TARGET).1


