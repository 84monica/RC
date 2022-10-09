// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>

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

    // // OPEN FILE FOR READING
    // int file_fd;
    // file_fd = open(filename, "r");

    // // GET FILE SIZE
    // struct stat fileStats;

    // if (stat(filename, &fileStats) == -1) {
    //     perror("stat");
    // }

    // printf("Inode number: %lu\n", fileStats.st_ino);
    // printf("User ID of owner: %u\n", fileStats.st_uid);
    // printf("Group ID of owner: %u\n", fileStats.st_gid);
    // printf("Total file size: %lu bytes\n", fileStats.st_size);
    // printf("Last status change:       %s", ctime(&fileStats.st_ctime));
    // printf("Last file access:         %s", ctime(&fileStats.st_atime));
    // printf("Last file modification:   %s", ctime(&fileStats.st_mtime));
    
    // // READ FROM FILE
    // unsigned char buf[8];
    // int bytes = read(file_fd, buf, 8);

    // // CLOSE FILE FOR READING
}

void receiveFile() {
    unsigned char packet[BUF_SIZE];
    llread(packet);
}