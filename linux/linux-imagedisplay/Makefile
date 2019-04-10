APP = linux-imagedisplay
WAYLAND=`pkg-config wayland-client --cflags --libs`
#CFLAGS?=-std=c11 -Wall -Werror -O3 -fvisibility=hidden -g
SOURCE = main.c  util.c
IMAGE = 640x480.bmp
CC ?= gcc

all: $(APP)

$(APP): ${SOURCE}
	$(CC)  -c ${SOURCE}
	$(CC) -o $(APP) *.o $(WAYLAND) -lrt
	
install:
	install -D -m755 $(APP) $(DESTDIR)/home/root/$(APP)
	install -D -m755 $(IMAGE) $(DESTDIR)/home/root/$(IMAGE)

clean:
	rm -rf $(APP) *.o
