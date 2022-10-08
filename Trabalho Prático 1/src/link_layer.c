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
#define A_UA 0x01    // Comandos enviados pelo Receptor e Respostas enviadas pelo Emissor
// Campo de Controlo
// Define o tipo de trama 
#define C_SET 0x03 
#define C_UA 0x07
#define C_0 0x00
#define C_1 0x40
// Campo de Proteção
#define BCC_SET (A_SET ^ C_SET)
#define BCC_UA (A_UA ^ A_UA)
#define BUF_SIZE 256


int fd;
int alarmEnabled = FALSE;
int alarmCount = 0;
int trama_0 = TRUE;

LinkLayer connection_parameters; 


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
    printf("START LLOPEN ---------------------------------------\n");

    // save conectionParameters for later usage
    connection_parameters = connectionParameters;

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

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
                    alarmCount = 0;
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
    printf("FINISHED LLOPEN ---------------------------------------\n");
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    printf("START LLWRITE ---------------------------------------\n");

    // GENERATE BCC2
    unsigned char BCC2 = buf[0];
    for (int i = 1; i < bufSize; i++) {
        BCC2 ^= buf[i];
    }

    // BYTE STUFFING 
    int counter = 0;
    for (int i = 0; i < bufSize; i++) {
        // find out how many 0x7E or 0x7D are present in buf
        if (buf[i] == 0x7E || buf[i] == 0x7D) counter++;
    }
    // check bcc2
    if (BCC2 == 0x7E || BCC2 == 0x7D) counter++;
    unsigned char buf_after_byte_stuffing[bufSize+counter];
    // replace 0x7E and 0x7D
    for (int i = 0, j = 0; i < bufSize; i++) {
        if (buf[i] == 0x7E) {
            buf_after_byte_stuffing[j] = 0x7D;
            buf_after_byte_stuffing[j+1] = 0x5E;
            j+=2;
        }
        else if (buf[i] == 0x7D) {
            buf_after_byte_stuffing[j] = 0x7D;
            buf_after_byte_stuffing[j+1] = 0x5D;
            j+=2;
        }
        else {
            buf_after_byte_stuffing[j] = buf[i];
            j++;
        }
    }

    // CREATE INFORMATION MESSAGE
    unsigned char packet_to_send[bufSize+counter+6];
    packet_to_send[0] = FLAG;
    packet_to_send[1] = A_SET;
    switch (trama_0)
    {
    case TRUE:
        packet_to_send[2] = C_0;
        break;
    case FALSE:
        packet_to_send[2] = C_1;
        break;
    default:
        break;
    }
    packet_to_send[3] = packet_to_send[1] ^ packet_to_send[2];

    // add buf to information message
    for (int i = 4, j = 0; i < bufSize+counter+4; i++, j++) {
        packet_to_send[i] = buf_after_byte_stuffing[j];
    }
    // byte stuffing for bcc2
    if (BCC2 == 0x7E) {
        packet_to_send[bufSize+counter+3] = 0x7D;
        packet_to_send[bufSize+counter+4] = 0x5E;
        packet_to_send[bufSize+counter+5] = FLAG;
    }
    else if (BCC2 == 0x7D) {
        packet_to_send[bufSize+counter+3] = 0x7D;
        packet_to_send[bufSize+counter+4] = 0x5D;
        packet_to_send[bufSize+counter+5] = FLAG;
    }
    else {
        packet_to_send[bufSize+counter+4] = BCC2;
        packet_to_send[bufSize+counter+5] = FLAG;
    }

    // CHECK MESSAGE TO SEND
    // for (int i = 0; i < bufSize+counter+6; i++) {
    //     printf("%hx\n", packet_to_send[i]);
    // }

    // SEND INFORMATION MESSAGE
    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    // sends message at most 3 times
    while (alarmCount <= connection_parameters.nRetransmissions)
    {
        // SEND INFORMATION PACKET
        int bytes = write(fd, packet_to_send, sizeof(packet_to_send));
        printf("INFORMATION PACKET SENT - %d bytes written\n", bytes);

        // sets alarm of 3 seconds
        if (alarmEnabled == FALSE)
        {
            alarm(connection_parameters.timeout); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        // READ RESPONSE
        // TO DO ----------------------------------------------------------------------
        unsigned char response[BUF_SIZE];
        int i = 0;
        int STATE = 0;
        while (STATE != 5)
        {
            int bytes = read(fd, response + i, 1);
            //printf("%hx %d\n", response[i], bytes);
            if (bytes == -1) break;
            if (bytes > 0) {
                // STATE MACHINE
                switch (STATE)
                {
                case 0:
                    if (response[i] == FLAG) STATE = 1;
                    break;
                case 1:
                    if (response[i] == FLAG) STATE = 1;
                    if (response[i] == A_UA) STATE = 2;
                    else STATE = 0;
                    break;
                case 2:
                    if (response[i] == FLAG) STATE = 1;
                    if (response[i] == C_UA) STATE = 3;
                    else STATE = 0;
                    break;
                case 3:
                    if (response[i] == FLAG) STATE = 1;
                    if (response[i] == BCC_UA) STATE = 4;
                    else STATE = 0;
                    break;
                case 4:
                    if (response[i] == FLAG) STATE = 5;
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
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    printf("START LLREAD ---------------------------------------\n");

    // READ INFORMATION PACKET
    unsigned char buf[BUF_SIZE];
    int i = 0, size_of_buf = 0;
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
                if (trama_0 && buf[i] == C_0) STATE = 3;
                else if (!trama_0 && buf[i] == C_0) STATE = 3;
                else STATE = 0;
                break;
            case 3:
                if (buf[i] == FLAG) STATE = 1;
                if (buf[i] == (buf[i-1] ^ buf[i-2])) {
                    STATE = 4;
                    i = -1;
                }
                else STATE = 0;
                break;
            case 4:
                if (buf[i] == FLAG) STATE = 5;
                else {
                    size_of_buf++;
                }
                break;
            
            default:
                break;
            }
            i++; 
        }
    }
    // for (int i = 0; i < size_of_buf; i++) {
    //     printf("%hx\n", buf[i]);
    // }

    // BYTE DESTUFFING 
    // TO DO --------------------------------------

    // CHECK BCC2
    // TO DO --------------------------------------

    printf("PACKET RECEIVED\n");

    // SEND RR MESSAGE
    // TO DO --------------------------------------
    
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
