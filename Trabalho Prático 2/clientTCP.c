/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <ctype.h>

#define FALSE 0
#define TRUE 1
#define SERVER_PORT 21
#define SERVER_ADDR "193.137.29.15"

void readResponse(int sockfd) {
    int digit_counter = 0; int end_read=FALSE;
    while ( TRUE )
    {
        // read char
        char ch;
        size_t bytes = read(sockfd, &ch, 1);
        if (bytes > 0) {
            printf("%c", ch);
        }
        else {
            perror("read()");
            exit(-1);
        }

        // get code received
        if (isdigit(ch)) {
            digit_counter++;
            continue;
        }
        
        // end reading response condition
        if (digit_counter == 3 && ch == ' ') {
            end_read = TRUE;
            digit_counter = 0;
        }
        if (end_read == TRUE && ch == '\n') {
            break;
        }
        // not end reading response condition
        if (digit_counter == 3 && ch != ' ') {
            digit_counter = 0;
        }
    }
}

int getPort(int sockfd) {
    int digit_counter = 0; int end_read=FALSE;

    char port[4]; char port1[4]; 
    int byte_received_counter = 0;
    
    while ( TRUE )
    {
        // read char
        char ch;
        size_t bytes = read(sockfd, &ch, 1);
        if (bytes > 0) {
            printf("%c", ch);
        }
        else {
            perror("read()");
            exit(-1);
        }

        // get code received
        if (isdigit(ch)) {
            if (byte_received_counter == 4) {
                port[digit_counter] = ch;
            }
            else if (byte_received_counter == 5) {
                port1[digit_counter] = ch;
            }
            digit_counter++;
            continue;
        }
        
        // end reading response condition
        if (digit_counter == 3 && ch == ' ') {
            end_read = TRUE;
            digit_counter = 0;
        }
        if (end_read == TRUE && ch == '\n') {
            break;
        }
        // get port number condition
        if (end_read == TRUE && ch == ',') {
            if (byte_received_counter == 4) {
                port[digit_counter] = '\0';
            }
            else if (byte_received_counter == 5) {
                port1[digit_counter] = '\0';
            }
            byte_received_counter++;
            digit_counter = 0;
        }
        // not end reading response condition
        if (digit_counter == 3 && ch != ' ') {
            digit_counter = 0;
        }
    }
    return (atoi(port) * 256 + atoi(port1));
}

int main(int argc, char **argv) {

    // if (argc != 2) {
    //     printf("Error Argmuments.\n");
    //     return -1;
    // }
    /*get arguments*/

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    readResponse(sockfd);

    /*loggin in server*/
    char user[] = "user anonymous\n";
    size_t bytes = write(sockfd, user, strlen(user));
    if (bytes > 0)
        printf("Bytes escritos %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }
    readResponse(sockfd);

    char pass[] = "pass qualquer-password\n";
    bytes = write(sockfd, pass, strlen(pass));
    if (bytes > 0)
        printf("Bytes escritos %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }
    readResponse(sockfd);

    /*passv command*/
    char pasv[] = "pasv\n";
    bytes = write(sockfd, pasv, strlen(pasv));
    if (bytes > 0)
        printf("Bytes escritos %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }
    int port = getPort(sockfd);

    /*open new connection to server in port received*/
    /*server address handling*/
    int new_sockfd;
    struct sockaddr_in new_server_addr;
    bzero((char *) &new_server_addr, sizeof(new_server_addr));
    new_server_addr.sin_family = AF_INET;
    new_server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);    /*32 bit Internet address network byte ordered*/
    new_server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((new_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(new_sockfd,
                (struct sockaddr *) &new_server_addr,
                sizeof(new_server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    /*retr command*/
    char retr[] = "retr pub/kodi/timestamp.txt\n";
    bytes = write(sockfd, retr, strlen(retr));
    if (bytes > 0)
        printf("Bytes escritos %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }
    readResponse(sockfd);
    readResponse(sockfd);

    /*get file*/
    FILE *file = fopen((char *)"timestamp.txt", "wb+");

	char buf[100];
 	while ((bytes = read(new_sockfd, buf, 100))>0) {
    	bytes = fwrite(buf, bytes, 1, file);
    }

  	fclose(file);

    if (close(sockfd)<0) {
        perror("close()");
        exit(-1);
    }
    return 0;
}


