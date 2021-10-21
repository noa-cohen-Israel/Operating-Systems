#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <sys/time.h>



void write_bytes(int fd, char *src, int size){
    int totalsent = 0;
    int nsent = -1;

    while (size > 0) {

        nsent = write(fd, src + totalsent, size);
      
        if(nsent < 0){
            fprintf(stderr, "\n Error : Cannot write. %s \n", strerror(errno));
            close(fd);
        }
        totalsent += nsent;
        size -= nsent;
    }
}

int main(int argc, char *argv[])
{
    int  sockfd     = -1;
    int  bytes_read =  0;

    struct sockaddr_in serv_addr;

    if(argc != 4){
        fprintf(stderr, "\n Error : Invalid Number Of Arguments. \n");
        exit(1);
    }

    // file opening
    FILE *fp = fopen(argv[3], "r");
    if (fp == NULL) {
        fprintf(stderr, "\n Error : Cannot open file. %s \n", strerror(errno));
        exit(1);
    }

    fseek(fp, 0, SEEK_END); // seek to end of file
    long size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET);

    char *send_buff = malloc(size + 1);
    if(send_buff == NULL){
        fprintf(stderr, "\n Error : Allocation Failed. %s \n", strerror(errno));
        exit(1);
    }

    // creating socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "\n Error : Could not create socket %s\n", strerror(errno));
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
//    inet_aton(argv[1], &serv_addr.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "\n Error : Connect Failed. %s \n", strerror(errno));
        exit(1);
    }


    // step 1
    uint32_t n = htonl(size);

    write_bytes(sockfd, (char*)&n, sizeof(n));


    // step 2
    char ch;
    while((ch = getc(fp)) != EOF)
    {
        strcat(send_buff, &ch);
    }

    if (0 != ferror(fp))
    {
        fprintf(stderr, "\n Error : getc() Failed. %s \n", strerror(errno));
        exit(1);
    }

    write_bytes(sockfd, send_buff, strlen(send_buff));

    // step 3
    uint32_t num_printable;
    while(1)
    {
        bytes_read = read(sockfd, &num_printable, sizeof(num_printable));

        if( bytes_read <= 0 )
            break;
    }
    
    printf("# of printable characters: %u\n", htonl(num_printable));
    

    free(send_buff);
    fclose(fp);
    close(sockfd);

    return 0;
}
