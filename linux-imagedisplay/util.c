#include "util.h"

#define WHITE           0xffff      /* 255, 255, 255 */
#define SIZEOF_CHAR     1

/* Define main var as global var for simplicity */
struct wl_shm *shm=NULL;

/* Convert bmp file to raw data */
int write_bmp_data(const char *filename,const char *outfile)
{
    FILE *infp = NULL;
    FILE *outfp = NULL;
    int rCode, l, i;
    int B, G, R;
    int x = WHITE;

    /* Open file to read and write */
    infp = fopen(filename, "r");
    if (infp == NULL) {
        puts("Cannot open input file");
        return -1;
    }

    outfp = fopen(outfile, "w");
    if (outfp == NULL) {
        puts("Cannot create output file");
        fclose(infp);
        return -1;
    }

    /* Read file header */
    rCode = fread(&bfh, SIZEOF_CHAR, sizeof(BITMAPFILEHEADER), infp);
    if (rCode < sizeof(BITMAPFILEHEADER)) {
        puts("ERROR - Read file header fail");
        return -1;
    }
    rCode = fread(&bih, SIZEOF_CHAR, sizeof(BITMAPINFOHEADER), infp);
    if (rCode < sizeof(BITMAPINFOHEADER)) {
        puts("ERROR - Read info header fail");
        return -1;
    }

    /* Write bmp data */
    l = bih.biWidth * abs(bih.biHeight);
    for (i = 0; i < l; i++) {   /* Only read one frame */
        /* BMP format use BGR */
        B = fgetc(infp);
        G = fgetc(infp);
        R = fgetc(infp);
        if ((B == EOF) || (G == EOF) || (R == EOF))
            break;

        fputc(B, outfp);
        fputc(G, outfp);
        fputc(R, outfp);
        fputc(x, outfp);
    }

    fclose(infp);
    fclose(outfp);

    return 0;
}

/* Create mem pool */
static struct wl_shm_pool *mem_pool_create(int file)
{
    struct pool_data *data;
    struct wl_shm_pool *pool;
    struct stat stat;

    if (fstat(file, &stat) != 0)
        return NULL;

    data = malloc(sizeof(struct pool_data));
    if (data == NULL)
        return NULL;

    data->capacity = stat.st_size;
    data->size = 0;
    data->fd = file;

    data->memory = mmap(0, data->capacity, PROT_READ, MAP_SHARED, data->fd, 0);

    if (data->memory == MAP_FAILED)
        goto cleanup_alloc;

    pool = wl_shm_create_pool(shm, data->fd, data->capacity);

    if (pool == NULL)
        goto cleanup_mmap;

    wl_shm_pool_set_user_data(pool, data);

    return pool;

  cleanup_mmap:
    munmap(data->memory, data->capacity);
  cleanup_alloc:
    free(data);
    return NULL;
}

/* Create image buffer */
struct wl_buffer *create_image_buffer(int image)
{
    struct pool_data *pool_data;
    struct wl_shm_pool *pool;
    struct wl_buffer *buffer;
    unsigned width,  height;

    /* Init data */
    width = bih.biWidth;
    height = abs(bih.biHeight);

    /* Create mem pool */
    pool = mem_pool_create(image);
    pool_data = wl_shm_pool_get_user_data(pool);
    buffer = wl_shm_pool_create_buffer(pool,
                                       pool_data->size, width, height,
                                       width * sizeof(pixel), WL_SHM_FORMAT_ARGB8888);

    if (buffer == NULL)
        return NULL;

    pool_data->size += width * height * sizeof(pixel);
    
    /* Free mem pool */
    wl_shm_pool_destroy(pool);
    munmap(pool_data->memory, pool_data->capacity);
    free(pool_data);
    
    return buffer;
}

/* Free surface */
void free_surface(struct wl_shell_surface *shell_surface)
{
    struct wl_surface *surface;

    surface = wl_shell_surface_get_user_data(shell_surface);
    wl_shell_surface_destroy(shell_surface);
    wl_surface_destroy(surface);
}
