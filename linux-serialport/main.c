#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define LENGTH_OF_SENDING_STRING 6                /* length of sending string */
#define LENGTH_OF_SENDING_STRING_WITH_LINE_FEED 7 /* length of sending string with Line Feed */
#define LENGTH_OF_RECEIVING_STRING 9              /* length of receiving string */
#define LINE_FEED 0x0a                            /* Line Feed */
#define GET_SERIAL_PORT_COMMAND    "dmesg | grep ttyS  > /home/root/serial_info.txt"
#define SERIAL_PORT_INFO_FILE      "/home/root/serial_info.txt"

static void get_serial_port(char* serial_port) {

        /* Initial variables */
        char *line = NULL;
        size_t len, read;
        FILE *fp;

        system(GET_SERIAL_PORT_COMMAND);
        fp = fopen(SERIAL_PORT_INFO_FILE, "rt");

        if(fp == NULL) {
                printf("Can't open serial info file.\n");
                exit(1);
        } else {
                while((read = getline(&line, &len, fp)) != -1 ) {
                        char* p_start_serial   = strstr(line, "serial");
			if(p_start_serial != NULL) {
                        	char* p_end_serial     = strstr(line, "at");
				strcpy(serial_port, "/dev/"); 
                        	strncat(serial_port, p_start_serial+8, p_end_serial - 9 - p_start_serial);
			}
                }
        }
        fclose(fp);
}


void openport(void);
void readport(void);
int fd;

/* Open serial port */
void openport(void) {
    char device[50];
    struct termios oldtp, newtp;
    
    char serial_port[20];
    memset(serial_port, '\0', sizeof(serial_port)); 
    get_serial_port(serial_port);
    printf("Opening serial port: %s \n", serial_port);
    
    fd = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);    /* File descriptor for the port */
    write(fd, "Send: ", LENGTH_OF_SENDING_STRING);
    if (fd < 0) {
        perror(device);         /* Could not open the port */
    }
    
    /* Reading data from the Port*/
    fcntl(fd, F_SETFL, 0);
    tcgetattr(fd, &oldtp);      /* save current serial port settings */
    bzero(&newtp, sizeof(newtp));

    /* Set the baud rates to 115200 */
    cfsetispeed(&newtp, B115200);
    cfsetospeed(&newtp, B115200);

    newtp.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;     /* Enable receiver and set local mode */
    newtp.c_iflag = IGNPAR | ICRNL;
    newtp.c_oflag = 0;
    newtp.c_lflag = 0;

    newtp.c_cc[VINTR] = 0;      /* Ctrl-c */
    newtp.c_cc[VQUIT] = 0;      /* Ctrl-\ */
    newtp.c_cc[VERASE] = 0;     /* del */
    newtp.c_cc[VKILL] = 0;      /* @ */
    newtp.c_cc[VEOF] = 0;       /* Ctrl-d */
    newtp.c_cc[VTIME] = 0;      /* inter-character timer unused */
    newtp.c_cc[VMIN] = 0;       /* blocking or non blocking read until 1 character arrives */
}

/* Read/Write data from the port */
void readport(void) {
    char buff;
    char cmd[512];
    int n1, n2;
    int count = LENGTH_OF_RECEIVING_STRING;

    strcpy(cmd, "Recieve: ");
    while (1) {
        n1 = read(fd, &buff, 1);      /* Read each byte */
        if (n1 < 0) {
            fputs("Read failed!\n", stderr);
            break;
        } else if (n1 == 0)
            break;
        cmd[count] = buff;
        count++;
        if (buff == LINE_FEED) {
            n2 = write(fd, cmd, count);
            if (n2 < 0)
                fputs("Write() of bytes failed!\n", stderr);
            write(fd, "\nSend: ", LENGTH_OF_SENDING_STRING_WITH_LINE_FEED);
            count = LENGTH_OF_RECEIVING_STRING;
        }
    }
    close(fd);                  /* Close a serial port */
}

int main(int argc, char **argv) { 
    openport();
    readport();
    return 0;
}
