#include "util.h"

#define ARG_NUMBER              2            /* Number input parameters on terminal */
#define RAW_IMAGE_FILENAME     "images.bin" /* Default output name */

struct wl_display *display=NULL;
struct wl_shell *shell=NULL;
struct wl_registry *registry=NULL;
struct wl_buffer *buffer=NULL;
struct wl_surface *surface=NULL;
struct wl_shell_surface *shell_surface=NULL;
struct wl_compositor *compositor=NULL;

static void registry_global(void *data,
                            struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0)
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    else if (strcmp(interface, wl_shm_interface.name) == 0)
        shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    else if (strcmp(interface, wl_shell_interface.name) == 0)
        shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
}

/* registry callback for listener */    
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
};

static void shell_surface_ping(void *data,
                               struct wl_shell_surface *shell_surface,
                               uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    .ping = shell_surface_ping,
};

int main(int argc, char **argv)
{
    const char *filename;
    const char *raw_image_name = RAW_IMAGE_FILENAME;
    int image;
    int ret = 0;

    if (argc != ARG_NUMBER) {
        puts("Not enough arguments");
        puts("Usage:");
        puts("         ./linux-imagedisplay <filename>");
        puts("Example:");
        puts("         ./linux-imagedisplay 640x480.bmp");
        return -1;
    }

    filename = argv[1];

    /* Convert bmp file into bgrx8888 */
    ret = write_bmp_data(filename, raw_image_name);
    if (ret == -1) {
        perror("Input image processing failed!");
        return ret;
    }

    image = open(raw_image_name, O_RDWR);
    if (image < 0) {
        perror("Error opening surface image");
        return -1;
    }

    /* Connects to the display server */
    display = wl_display_connect(NULL);
    if (display == NULL) {
        perror("Connecting to display server failed!");
        close(image); 
        return -1;
    }

    /* Get registry to work with global object and set listener */  
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    /* Send request to server */
    wl_display_roundtrip(display);

    /* compositor is already bind in registry callback */    
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL)
        goto shell_destructor;

    shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
       goto surface_destructor;
    }

    /* shell_surface config */
    wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, 0);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_set_user_data(shell_surface, surface);
    wl_surface_set_user_data(surface, NULL);

    /* create and bind buffer to surface */
    buffer = create_image_buffer(image);  
    if (buffer == NULL) 
        goto surface_destructor;

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    while (1) {
        if (wl_display_dispatch(display) < 0) {
            perror("Main loop error");
            break;
        }
    }

    /* Clean up */
    wl_buffer_destroy(buffer);

surface_destructor:    
    free_surface(shell_surface);

shell_destructor:
    wl_shell_destroy(shell);

registry_destructor:
    wl_shm_destroy(shm);
    wl_compositor_destroy(compositor);
    wl_registry_destroy(registry);

    wl_display_disconnect(display);   
    close(image);    

    return 0;
}
