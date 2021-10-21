#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <inttypes.h>
#include <signal.h>

uint32_t pcc_total[95];
int has_finished;
int run_status;


void exit_server(){
    for(char c = 32; c <= 126; ++c){
        printf("char ’%c’ : %u times\n", c, pcc_total[c-32]);
    }

    exit(0);
}


void handle_sigint(){
    if(run_status){
        has_finished = 1;
        return;
    } else{
        exit_server();
    }
}


int main(int argc, char *argv[])
{
    int listenfd  = -1;
    int connfd    = -1;
    int  bytes_read =  0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    if(argc != 2){
        fprintf(stderr, "\n Error : Invalid Number Of Arguments. \n");
        exit(1);
    }

    signal(SIGINT, handle_sigint);

    listenfd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        printf("error(setsockopt(SO_REUSEADDR) failed)");
    }

    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if( 0 != bind(listenfd, (struct sockaddr*) &serv_addr, addrsize)){
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        exit(1);
    }

    if( 0 != listen( listenfd, 10 )){
        fprintf(stderr, "\n Error : Listen Failed. %s \n", strerror(errno));
        exit(1);
    }

    while(1){

        connfd = accept( listenfd,
                         (struct sockaddr*) &peer_addr,
                         &addrsize);

        if(connfd < 0){
            fprintf(stderr, "\n Error : Accept Failed. %s \n", strerror(errno));
            exit(1);
        }

        int failed = 0;
        run_status = 1;

        // read size
        uint32_t n;
        bytes_read = read(connfd, &n, sizeof(n));
        if( bytes_read <= 0 ){
            fprintf(stderr, "\n Error : Cannot read. %s \n", strerror(errno));
            close(connfd);
            continue;
        }

        char *conn_buff = malloc(htonl(n)+1);
        if(conn_buff == NULL){
            fprintf(stderr, "\n Error : Allocation Failed. %s \n", strerror(errno));
            exit(1);
        }
        // read bytes
        bytes_read = read(connfd, conn_buff, htonl(n));

        if( bytes_read <= 0 ){
            fprintf(stderr, "\n Error : Cannot read. %s \n", strerror(errno));
            close(connfd);
            continue;
        }



        // count printable
        size_t data_len = strlen(conn_buff);
        uint32_t printable_count = 0;
        for(size_t i = 0; i < data_len; ++i){

            if(conn_buff[i] >= 32 && conn_buff[i] <= 126){
                ++printable_count;
            }
        }


        // return result
        int totalsent = -1;
        int nsent     = -1;
        int notwritten = sizeof(printable_count);

        while(notwritten > 0){
            nsent = write(connfd,  &printable_count + totalsent, notwritten);

            if( nsent < 0){
                fprintf(stderr, "\n Error : Cannot write. %s \n", strerror(errno));
                close(connfd);
                failed = 1;
                break;
            }

            totalsent  += nsent;
            notwritten -= nsent;
        }

        // update counters
        if(!failed){
            for(size_t i = 0; i < data_len; ++i){
                if(conn_buff[i] >= 32 && conn_buff[i] <= 126){
                    ++pcc_total[conn_buff[i]-32];
                }
            }
        }

        // close socket
        close(connfd);
        run_status = 0;

        if(has_finished){
            exit_server();
        }
    }
}
