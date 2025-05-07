#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_CNT     MAX_CACHE_SIZE / MAX_OBJECT_SIZE

typedef struct {
  char uri[MAXLINE];
  char object[MAX_OBJECT_SIZE];
  int size;
  int LRU;  // LRU 처리를 위한 필드
  int is_used;
  pthread_mutex_t lock; // 읽기용
} cache_block;

typedef struct {
  cache_block blocks[CACHE_CNT];
  pthread_mutex_t global_lock; // 쓰기 삽입 용
} Cache;

Cache cache;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void parse_url(char* uri, char *hostname, char *path, char *port);
void make_header(char *buf, const char *host, const char *path, const char *user_agent_hdr);
void thread_func(void *vargp);
void doit(int clientfd);
void init_cache(Cache *cache);
int cache_read(Cache *cache, char *uri, char *buf);
int choose_LRU_block(Cache *cache);
void cache_write(Cache *cache, char *uri, char *buf, int size);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



int main(int argc, char *argv[])
{
  int listenfd; 
  int *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  pthread_t tid; //thread_id


  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  init_cache(&cache);
  
  listenfd = open_listenfd(argv[1]);
  while(1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    pthread_create(&tid, NULL, thread_func, connfdp);
    }

}

void thread_func(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  free(vargp);

  doit(connfd);

  Close(connfd);
  return NULL;
}

void doit(int clientfd)
{
    int proxyfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE], port[MAXLINE], hostname[MAXLINE];
    char path[MAXLINE], header_buf[MAX_OBJECT_SIZE];
    rio_t rio_server, rio_client;

    Rio_readinitb(&rio_client, clientfd);
    Rio_readlineb(&rio_client, buf, MAX_OBJECT_SIZE);

    sscanf(buf, "%s %s %s", method, uri, version);
    parse_url(uri, hostname, path, port);

    // 1. 캐시 확인
    int cached_size = cache_read(&cache, uri, buf);
    if (cached_size != -1) {
        Rio_writen(clientfd, buf, cached_size);
        return;
    }

    // 2. 캐시 미스 -> 서버에 요청
    proxyfd = Open_clientfd(hostname, port);
    Rio_readinitb(&rio_server, proxyfd);
    make_header(header_buf, hostname, path, user_agent_hdr);
    Rio_writen(proxyfd, header_buf, strlen(header_buf));

    // 3. 응답 내용을 버퍼에 저장하며 클라이언트에 전송
    int n, total_size = 0;
    char cache_buf[MAX_OBJECT_SIZE];

    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0) {
        if (total_size + n <= MAX_OBJECT_SIZE) {
            memcpy(cache_buf + total_size, buf, n);
        }
        total_size += n;

        Rio_writen(clientfd, buf, n);
    }

    // 4. 응답 전체가 MAX_OBJECT_SIZE보다 작을 때만 캐시에 저장
    if (total_size <= MAX_OBJECT_SIZE) {
        cache_write(&cache, uri, cache_buf, total_size);
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

void init_cache(Cache *cache) {
  pthread_mutex_init(&cache->global_lock, NULL);
  for (int i = 0; i < CACHE_CNT; i++) {
      cache->blocks[i].is_used = 0;
      cache->blocks[i].LRU = 0;
      pthread_mutex_init(&cache->blocks[i].lock, NULL);
  }
}
 
int cache_read(Cache *cache, char *uri, char *buf) {
  for (int i = 0; i < CACHE_CNT; i++) {
      pthread_mutex_lock(&cache->blocks[i].lock);

      if (cache->blocks[i].is_used) {
          cache->blocks[i].LRU++; // 사용 안 하면 LRU 증가

          if (strcmp(uri, cache->blocks[i].uri) == 0) {
              memcpy(buf, cache->blocks[i].object, cache->blocks[i].size);
              cache->blocks[i].LRU = 0; // 최근 사용됨
              pthread_mutex_unlock(&cache->blocks[i].lock);
              return cache->blocks[i].size;
          }
      }

      pthread_mutex_unlock(&cache->blocks[i].lock);
  }

  return -1;
}

int choose_LRU_block(Cache *cache) {
  int max_LRU = -1;
  int victim = -1;

  for (int i = 0; i < CACHE_CNT; i++) {
      if (!cache->blocks[i].is_used) return i;
      if (cache->blocks[i].LRU > max_LRU) {
          max_LRU = cache->blocks[i].LRU;
          victim = i;
      }
  }

  return victim;
}

void cache_write(Cache *cache, char *uri, char *buf, int size) {
  if (size > MAX_OBJECT_SIZE) return;

  pthread_mutex_lock(&cache->global_lock);
  int victim = choose_LRU_block(cache);

  pthread_mutex_lock(&cache->blocks[victim].lock);
  strncpy(cache->blocks[victim].uri, uri, MAXLINE);
  memcpy(cache->blocks[victim].object, buf, size);
  cache->blocks[victim].size = size;
  cache->blocks[victim].is_used = 1;
  cache->blocks[victim].LRU = 0;
  pthread_mutex_unlock(&cache->blocks[victim].lock);

  pthread_mutex_unlock(&cache->global_lock);
}

