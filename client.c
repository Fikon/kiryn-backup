/*
** test on the light server
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "4399"
#define MAXDATASIZE 100

using namespace std;


int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    int s_sock;

    if (argc != 2) {
        cout<<"usage: client hostname"<<endl;
        exit(1);
    }

    bzero(&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        cout<<"get addrinfo error"<<endl;
        exit(2);
    }


    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {

            continue;
        }

        if((s_sock = connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        cout<<"no addr"<<endl;
        exit(3);
    }

    freeaddrinfo(servinfo);
    fd_set readset;
    FD_ZERO(&readset);
    char rbuf[1024];
    char wbuf[1024];
    int maxfd;
    int nfds;
    int res;
    int stdinfno = fileno(stdin);
    if(stdinfno > sockfd)
      maxfd = stdinfno;
    else
      maxfd = sockfd;

    while(1){
      FD_SET(stdinfno, &readset);
      FD_SET(sockfd, &readset);
      nfds = select(maxfd + 1, &readset, NULL, NULL, NULL);
      if(nfds == -1)
        break;
      else if(nfds == 0)
        continue;
      else{
        if(FD_ISSET(sockfd, &readset)){
          bzero(&rbuf, sizeof rbuf);
          res = read(sockfd, rbuf, sizeof(rbuf));
          if(res == -1){
            cout<<"read error"<<endl;
            continue;
          }
          else if(res == 0){
            cout<<"server close"<<endl;
            close(sockfd);
            break;
          }
          else{
            cout<<"echo: "<<rbuf;
          }
        }
        if(FD_ISSET(stdinfno, &readset)){
          fgets(wbuf, sizeof wbuf, stdin);
          write(sockfd, wbuf, sizeof(wbuf));
        }
      }
    }

    return 0;
}
