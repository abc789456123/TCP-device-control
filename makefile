# Makefile
CC = gcc
CFLAGS = -Wall -g -fPIC -DLIBDIR=\"./serverLib\"
LDFLAGS = -lwiringPi -ldl -lpthread

LIBDIR = ./serverLib
LIBS = libled.so libcds.so libbuzzer.so libfnd.so

.PHONY: all clean

all: $(LIBDIR) $(addprefix $(LIBDIR)/, $(LIBS)) server client

$(LIBDIR):
	mkdir -p $(LIBDIR)

$(LIBDIR)/libled.so: led.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lwiringPi

$(LIBDIR)/libcds.so: cds.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lwiringPi

$(LIBDIR)/libbuzzer.so: buzzer.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lwiringPi

$(LIBDIR)/libfnd.so: fnd.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lwiringPi

server: server.c
	$(CC) $(CFLAGS) -rdynamic -o $@ server.c $(LDFLAGS)

client: client.c
	$(CC) -Wall -g -o client client.c

clean:
	rm -f server client *.o $(addprefix $(LIBDIR)/, $(LIBS))
