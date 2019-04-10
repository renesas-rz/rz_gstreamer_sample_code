#ifndef _UTIL_H
#define _UTIL_H
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <wayland-client.h>

typedef uint32_t pixel;

typedef struct tagBITMAPFILEHEADER {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER;

BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

struct pool_data {
    int fd;
    pixel *memory;
    unsigned capacity;
    unsigned size;
};

extern struct wl_shm *shm;

/* Function declaration */
int write_bmp_data(const char *filename, const char *outfile);

struct wl_buffer *create_image_buffer(int image);

void free_surface(struct wl_shell_surface *shell_surface);


#endif /* _UTIL_H */
