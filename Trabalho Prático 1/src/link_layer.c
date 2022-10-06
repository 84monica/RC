// Link layer protocol implementation

#include "link_layer.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>


// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
struct termios oldtio;
struct termios newtio;


#define FLAG 0x7E
// Campo de Enderesso
#define A_SET 0x03    // Comandos enviados pelo Emissor e Respostas enviadas pelo Receptor
#define A_UA 0x03    // Comandos enviados pelo Receptor e Respostas enviadas pelo Emissor
// Campo de Controlo
#define C_SET 0x03  // Define o tipo de trama 
#define C_UA 0x07
// Campo de Proteção
#define BCC_SET (A_SET ^ C_SET)
#define BCC_UA (A_UA ^ A_UA)
#define BUF_SIZE 256


int fd;
int alarmEnabled = FALSE;
int alarmCount = 0;


// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
    return;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

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
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

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

    // SEND/RECEIVE MESSAGES
    switch (connectionParameters.role)
    {
    // TRANSMITER
    case LlTx:
        {
            // CREATE SET MESSAGE
            unsigned char set_message[BUF_SIZE];
            set_message[0] = FLAG;
            set_message[1] = A_SET;
            set_message[2] = C_SET;
            set_message[3] = BCC_SET;
            set_message[4] = FLAG;

            // Set alarm function handler
            (void)signal(SIGALRM, alarmHandler);

            // sends message at most 3 times
            while (alarmCount <= connectionParameters.nRetransmissions)
            {
                // SEND SET MESSAGE
                int bytes = write(fd, set_message, 5);
                printf("SET MESSAGE SENT - %d bytes written\n", bytes);
                sleep(1);

                // sets alarm of 3 seconds
                if (alarmEnabled == FALSE)
                {
                    alarm(connectionParameters.timeout); // Set alarm to be triggered in 3s
                    alarmEnabled = TRUE;
                }

                // READ UA MESSAGE
                unsigned char buf[BUF_SIZE];
                int i = 0;
                int STATE = 0;
                while (STATE != 5)
                {
                    int bytes = read(fd, buf + i, 1);
                    //printf("%hx %d\n", buf[i], bytes);
                    if (bytes == -1) break;
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
                    // timeout
                    if (alarmEnabled == FALSE) break;
                }
                
                // RECEIVED UA MESSAGE
                if (STATE == 5) {
                    printf("UA RECEIVED\n");
                    break;
                }
            }
        }
        break;
    // RECEIVER
    case LlRx:
        {
            // READ SET MESSAGE
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
                        if (buf[i] == A_SET) STATE = 2;
                        else STATE = 0;
                        break;
                    case 2:
                        if (buf[i] == FLAG) STATE = 1;
                        if (buf[i] == C_SET) STATE = 3;
                        else STATE = 0;
                        break;
                    case 3:
                        if (buf[i] == FLAG) STATE = 1;
                        if (buf[i] == BCC_SET) STATE = 4;
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
            printf("SET RECEIVED\n");

            // SEND UA MESSAGE
            unsigned char ua_message[BUF_SIZE];
            ua_message[0] = FLAG;
            ua_message[1] = A_UA;
            ua_message[2] = C_UA;
            ua_message[3] = BCC_UA;
            ua_message[4] = FLAG;

            int bytes = write(fd, ua_message, 5);
            printf("UA MESSAGE SENT - %d bytes written\n", bytes);
        }
        break;
    default:
        break;
    }
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
