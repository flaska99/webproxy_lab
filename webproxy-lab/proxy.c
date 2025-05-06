#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void parse_url(char* uri, char *hostname, char *path, char *port);
void make_header(char *buf, const char *host, const char *path, const char *user_agent_hdr);


int main(int argc, char *argv[])
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];


  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  listenfd = open_listenfd(argv[1]);
  while(1) {
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    doit(connfd);
    Close(connfd);
    }

}


void doit(int clientfd)
{
  int proxyfd;
  char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], port[MAXLINE], hostname[MAXLINE];
  char path[MAXLINE], header_buf[MAX_OBJECT_SIZE];
  rio_t rio_server, rio_client;

  Rio_readinitb(&rio_client, clientfd);
  Rio_readlineb(&rio_client, buf, MAX_OBJECT_SIZE);

  sscanf(buf, "%s %s %s", method, uri, version);
  parse_url(uri, hostname, path, port);

  proxyfd = Open_clientfd(hostname, port);
  Rio_readinitb(&rio_server, proxyfd);
  make_header(header_buf, hostname, path, user_agent_hdr);
  Rio_writen(proxyfd, header_buf, strlen(header_buf));
  
  int n;
  while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0) {
    Rio_writen(clientfd, buf, n);
  }

  Close(proxyfd);

}

void make_header(char *buf, const char *host, const char *path, const char *user_agent_hdr) 
{
    sprintf(buf,
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "%s"
        "Connection: close\r\n"
        "Proxy-Connection: close\r\n"
        "\r\n",
        path, host, user_agent_hdr);
}

void parse_url(char* uri, char *hostname, char *path, char *port)
{
  //"http://localhost:8080/index.html 를 하나하나 파싱한다 생각해보자."
  char *hostbegin;
  char *hostend;
  char *pathbegin;
  int len;

  if (strncasecmp(uri, "http://", 7) == 0){
    hostbegin = uri + 7;
  } else {
    hostbegin = uri;
  }

  pathbegin = strchr(hostbegin, '/');
  strcpy(path, pathbegin);

  len = pathbegin - hostbegin;
  char hostport[MAX_OBJECT_SIZE];
  strncpy(hostport, hostbegin, len);
  hostport[len] = '\0';
  char *colon = strchr(hostport, ':'); 

  if(colon) {
    *colon = '\0';
    strcpy(hostname, hostport);
    strcpy(port, colon + 1);
  } else {
    strcpy(hostname, hostport);
    strcpy(port, "80");
  }
}
