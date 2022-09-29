// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;


#define FLAG 0x7E
// Campo de Enderesso
#define A_SET 0x03    // Comandos enviados pelo Emissor e Respostas enviadas pelo Receptor
#define A_UA 0x03    // Comandos enviados pelo Receptor e Respostas enviadas pelo Emissor
// Campo de Controlo
#define C_SET 0x03  // Define o tipo de trama 
#define C_UA 0x07
// Campo de Proteção
#define BCC_SET (A_SET ^ C_SET)
#define BCC_UA (A_UA ^ A_SET)


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // CREATE SET MESSAGE
    unsigned char set_message[BUF_SIZE];
    set_message[0] = FLAG;
    set_message[1] = A_SET;
    set_message[2] = C_SET;
    set_message[3] = BCC_SET;
    set_message[4] = FLAG;

    int bytes = write(fd, set_message, 5);
    printf("SET MESSAGE SENT - %d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    // READ UA MESSAGE
    unsigned char buf[BUF_SIZE];
    int i = 0;
    int STATE = 0;
    while (STATE != 5)
    {
        int bytes = read(fd, buf + i, 1);
        //printf("%hx %d\n", buf[i], STATE);
        if (bytes > 0) {
            // STATE MACHINE
            switch (STATE)
            {
            case 0:
                if (buf[i] == FLAG) STATE = 1;
                break;
            case 1:
                if (buf[i] == FLAG) STATE = 1;
                if (buf[i] == A_UA) STATE = 2;
                else STATE = 0;
                break;
            case 2:
                if (buf[i] == FLAG) STATE = 1;
                if (buf[i] == C_UA) STATE = 3;
                else STATE = 0;
                break;
            case 3:
                if (buf[i] == FLAG) STATE = 1;
                if (buf[i] == BCC_UA) STATE = 4;
                else STATE = 0;
                break;
            case 4:
                if (buf[i] == FLAG) STATE = 5;
                else STATE = 0;
                break;
            
            default:
                break;
            }
            i++; 
        }
    }
    printf("UA RECEIVED\n");

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
