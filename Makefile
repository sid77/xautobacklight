CC = gcc
CFLAGS = -std=c99 -O2 -I /usr/X11R6/include -L /usr/X11R6/lib -lX11 -lXrandr
PREFIX = /usr/local

all: xautobacklight

xautobacklight:
	$(CC) $(CFLAGS) -o xautobacklight xautobacklight.c

clean:
	rm -f xautobacklight

install: xautobacklight
	install -d $(DESTDIR)/$(PREFIX)/bin
	install -m 0755 xautobacklight $(DESTDIR)/$(PREFIX)/bin/
