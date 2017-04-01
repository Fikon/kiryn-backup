/*
** author : kiky.jiang@gmail.com
** date : 05-27
** desc : A trial to http server~~~~~~~~~~~~~~~~~~~~~~~~
**
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#define PORT "4399"
#define BACKLOG 20
#define EV_NUM 20
#define EPOLL_SIZE 512
#define MAXDATASIZE 1024

using namespace std;


int initialize(){

   struct addrinfo hints;
   struct addrinfo *ai;
   struct addrinfo *p;
   int res;
   int listener;
   int reuse = 1;


   bzero(&hints, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;


   if((res = getaddrinfo(NULL, PORT, &hints, &ai)) != 0){
      cout<<"getaddrinfo error"<<endl;
      exit(1);
   }
   for(p = ai; p != NULL; p = p -> ai_next){
      if((listener = socket(p -> ai_family, p -> ai_socktype, p -> ai_protocol)) == -1){
        continue;
      }

      if((res = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))) == -1){
        cout<<"set socket option error"<<endl;
        exit(2);
      }
      if((res = bind(listener, p -> ai_addr, p -> ai_addrlen)) == -1){
        close(listener);
        continue;
      }
      break;
    }
    freeaddrinfo(ai);
    if(p == NULL){
      cout<<"initialize error"<<endl;
      exit(2);
    }
    return listener;
  }

void set_nonblock(int fd){
  if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0){
    cout<<"set nonblock error"<<endl;
  }
}





int main(){
  int server_sock;
  struct sockaddr_storage *client_addr;
  socklen_t addr_len;
  int client_sock;
  // use epoll
  struct epoll_event ev;
  struct epoll_event events[EV_NUM];
  int epfd;
  int nfds;
  int tmp_sock;
  int res;

  char buffer[MAXDATASIZE - 1];

  server_sock = initialize();

  if(listen(server_sock, BACKLOG) == -1){
    cout<<"listen error"<<endl;
    exit(3);
  }

  cout<<"the server is listening on port "<<PORT<<endl;

  epfd = epoll_create(EPOLL_SIZE);
  set_nonblock(server_sock);
  ev.data.fd = server_sock;
  ev.events = EPOLLIN | EPOLLET; //edge chufa  ||-_-
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &ev);

  while(1){
    nfds = epoll_wait(epfd, events, EV_NUM, 500);
    if(nfds == -1){
      cout<<"epoll wait error"<<endl;
      continue;
    }
    addr_len = sizeof client_addr;
    for(int i = 0; i < nfds; i++){
      // server_sock ready
      if(events[i].data.fd == server_sock){
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if(client_sock == -1){
          cout<<"accept error"<<endl;
          continue;
        }
        ev.data.fd = client_sock;
        set_nonblock(client_sock);
        ev.events = EPOLLIN | EPOLLET;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &ev) < 0){
          cout<<"epoll ctl error"<<endl;
        }
      }
      //read
      else if(events[i].events & EPOLLIN){
        tmp_sock = events[i].data.fd;
        if(tmp_sock < 0){
          continue;
        }
        if((res = read(tmp_sock, buffer, MAXDATASIZE)) < 0){
          cout<<"read error"<<endl;
          continue;
        }
        else if(res == 0){
          cout<<"client exit"<<endl;
          close(tmp_sock);
          epoll_ctl(epfd, EPOLL_CTL_DEL, tmp_sock, &events[i]);
          events[i].data.fd = -1;
        }
        else{
          cout<<"recieve: "<<buffer;
          write(tmp_sock, buffer, sizeof(buffer));
        }
      }
    }
  }
}
