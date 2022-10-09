// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BUF_SIZE 256

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
    const unsigned char buf[6] = {0x1, 0x2, 0x7D, 0x23, 0x0, 0x7E};
    llwrite(buf, 6);    

    // OPEN FILE FOR READING
    FILE *file;
    file = fopen(filename, "rb");

    // GET FILE SIZE
    struct stat stats;
    stat(filename, &stats);
    long filesize = stats.st_size;
    printf("File Size: %ld bytes \n", filesize);

    // READ FROM FILE
    unsigned char *filedata;
    filedata = (unsigned char *)malloc(filesize);
    fread(filedata, sizeof(unsigned char), filesize, file);

    // SEND FILE DATA BY CHUNKS

    // CLOSE FILE FOR READING
}

void receiveFile() {
    unsigned char packet[BUF_SIZE];
    llread(packet);
}