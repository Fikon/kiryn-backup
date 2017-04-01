/*
**  echo server
**  kiky.jiang@gmail.com
**
*/

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
#include <sys/epoll.h>
#include <sys/fcntl.h>


#define PORT "4399"
#define BACKLOG 20 // the max number of connection in queue
#define EV_NUM 20
#define EPOLL_SIZE 1024
#define MAXBUFFERSIZE 1024


// print error information
void error(const char * str){
  printf("Exception happened on %s\n", str);
  exit(1);
}

// set the socket to be nonblock
void set_nonblock(int fd){
  if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1){
    error("set nonblock");
  }
}

//initialize the server
int server_init(){

  struct addrinfo hints;
  struct addrinfo *ai;
  struct addrinfo *iter;
  int res;
  int reuse = 1;

  // describe the type of addr we want
  bzero(&hints, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL, PORT, &hints, &ai) != 0){
    error("get addrinfo");
  }
  for(iter = ai; iter != NULL; iter = iter -> ai_next){
    if((res = socket(iter -> ai_family, iter -> ai_socktype, iter -> ai_protocol)) == -1){
      continue;
    }
    // set the sockaddr be reusable
    if(setsockopt(res, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1){
      continue;
    }
    if(bind(res, iter -> ai_addr, iter -> ai_addrlen) == -1){
      continue;
    }
    break;
  }
  freeaddrinfo(ai);
  if(iter == NULL){
    error("sock initilization");
  }
  return res;
}

int main(int argc, char *argv[]){

  int s_sock; // server socket
  int c_sock; //client socket
  int res;
  char buffer[MAXBUFFERSIZE];
  struct sockaddr_storage *client_addr;
  socklen_t caddr_len;

  struct epoll_event ev;
  struct epoll_event events[EV_NUM];
  int epfd;
  int nfds;

  s_sock = server_init();
  printf("Server listening on part %s\n", PORT);

  if(listen(s_sock, BACKLOG) == -1){
    error("listening");
  }

  epfd = epoll_create(EPOLL_SIZE);
  if(epfd == -1){
    error("create epoll");
  }
  set_nonblock(s_sock);
  ev.data.fd = s_sock;
  ev.events = EPOLLIN | EPOLLET; //read event, edge triger ||-_-
  if(epoll_ctl(epfd, EPOLL_CTL_ADD, s_sock, &ev) == -1){
    error("register read event for server socket");
  }
  while(true){
    nfds = epoll_wait(epfd, events, EV_NUM, 500);
    if(nfds == -1){
      error("epoll wait");
    }
    for(int i = 0; i < nfds; i++){
      //server socket is ready, accept the incoming connections
      if(events[i].data.fd == s_sock){
        c_sock = accept(s_sock, (struct sockaddr *)&client_addr, &caddr_len);
        if(c_sock == -1){
          continue;
        }
        ev.data.fd = c_sock;
        set_nonblock(c_sock);
        ev.events = EPOLLIN | EPOLLET;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, c_sock, &ev) == -1){
          continue;
        }
      }
      else if(events[i].events && EPOLLIN){ //read events
        c_sock = events[i].data.fd;
        if(c_sock < 0){
          continue;
        }
        if((res = read(c_sock, buffer, MAXBUFFERSIZE)) < 0){
          continue;
        }
        else if(res == 0){
          printf("the client close the connection\n");
          close(c_sock);
          epoll_ctl(epfd, EPOLL_CTL_DEL, c_sock, &events[i]);
          events[i].data.fd = -1;
        }
        else{
          printf("recieve string from client: %s\n", buffer);
          write(c_sock, buffer, sizeof buffer);
        }
      }
    }
  }
}
