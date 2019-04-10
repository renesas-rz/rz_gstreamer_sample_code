PROGRAM = GST_VideoConvert

DEPENDENCY = gstreamer-1.0

CFLAGS = -Wall -g $(shell pkg-config $(DEPENDENCY) --cflags)
LIBS = $(shell pkg-config $(DEPENDENCY) --libs)
CC ?= gcc

SRC_FILES = main.c

.PHONY: clean install all

all: $(PROGRAM)

$(PROGRAM): $(SRC_FILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

install:
	install -D -m 755 $(PROGRAM) $(DESTDIR)/usr/local/bin/$(PROGRAM)

clean:
	rm -f $(PROGRAM)
