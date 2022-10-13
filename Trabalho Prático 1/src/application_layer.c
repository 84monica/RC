// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define BUF_SIZE 256
// Campo de Controlo
// Define o tipo de trama 
#define C_DADOS 0x01
#define C_START 0x02 
#define C_END 0x03

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // GET CONNECTION PARAMETERS
    LinkLayer connectionParameters;
    int i = 0;
    while (serialPort[i] != '\0')
    {
        connectionParameters.serialPort[i] = serialPort[i];
        i++;
    }
    connectionParameters.serialPort[i] = serialPort[i];
    if (role[0] == 't') connectionParameters.role = LlTx;
    else if (role[0] == 'r') connectionParameters.role = LlRx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    // CALL LLOPEN
    llopen(connectionParameters);

    switch (connectionParameters.role)
    {
    case LlTx:
        // SENDS FILE PACKET BY PACKET
        {
            sendFile(filename);
        }
        break;
    case LlRx:
        // RECEIVES FILE PACKET BY PACKET
        {
            receiveFile();
        }
    default:
        break;
    }

    // CALL LLCLOSE
}

void sendFile(const char *filename) {
    // OPEN FILE FOR READING
    FILE *file;
    file = fopen(filename, "rb");

    // GET FILE SIZE
    struct stat stats;
    stat(filename, &stats);
    size_t filesize = stats.st_size;
    printf("File Size: %ld bytes \n", filesize);
    printf("File Name: %s \n", filename);

    // READ FROM FILE
    unsigned char *filedata;
    filedata = (unsigned char *)malloc(filesize);
    fread(filedata, sizeof(unsigned char), filesize, file);

    // SEND TRAMA START
    unsigned char start[30];
    start[0] = C_START;

    // 0 - file size
    start[1] = 0x0;
    start[2] = sizeof(size_t);
    start[3] = (filesize >> 24) & 0xFF;
    start[4] = (filesize >> 16) & 0xFF;
    start[5] = (filesize >> 8) & 0xFF;
    start[6] = filesize & 0xFF;
    
    // 1 - file name
    start[7] = strlen(filename)+1; 
    int i = 8;
    for(int j = 0; j < strlen(filename)+1; j++, i++) {
        start[i] = filename[j];
    }

    llwrite(start, i);
    printf("START MESSAGE SENT - %d bytes written \n", i);

    // SEND FILE DATA BY CHUNKS
    

    // SEND TRAMA END


    // CLOSE FILE FOR READING
}

void receiveFile() {
    // RECEIVE TRAMA START
    unsigned char packet[BUF_SIZE];
    int bytes = llread(packet);
    if (bytes != -1) printf("START RECEIVED - %d bytes received \n", bytes);

    // check start value
    if (packet[0] != C_START) exit(-1);

    // get file size
    if (packet[1] != 0x0) exit(-1);
    size_t filesize = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | (packet[6]);

    // get file name 
    unsigned char filename[7];
    for(int j = 0, i = 8; j < packet[7]; j++, i++) {
        filename[j] = packet[i];
    }

    printf("File Size: %ld bytes\n", filesize);
    printf("File Name: %s \n", filename);

    // RECEIVE PACKET BY PACKET UNTIL TRAMA END
    

    // SAVE FILE
}