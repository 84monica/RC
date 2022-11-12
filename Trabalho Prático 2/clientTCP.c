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
    /*read response*/
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
    readResponse(sockfd);

    if (close(sockfd)<0) {
        perror("close()");
        exit(-1);
    }
    return 0;
}


