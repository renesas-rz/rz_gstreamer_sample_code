APP = linux-helloworld
SRC = main.c

all: $(APP)

CC ?= gcc

# Options for development
CFLAGS = -g -O0 -Wall -DDEBUG_LOG

$(APP):
	$(CC) -o $(APP) $(SRC) $(CFLAGS)

install:
	install -D -m755 $(APP) $(DESTDIR)/home/root/$(APP)

clean:
	rm -rf $(APP)
