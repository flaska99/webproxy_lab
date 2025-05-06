/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // 포트 인자로 Open_litenfd 함수 호출
  // 해당 포트로 오는 요청을 받을 수 있는 listen 소켓을 생성한다.
  listenfd = Open_listenfd(argv[1]); 
  while (1)
  {
    clientlen = sizeof(clientaddr);
    //해당 소켓으로 오는 요청에 대해 connfd 소켓을 생성한다.
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    
    // 로그 출력을 위해 Getnameinfo 함수로 구조체에 대한 hostname과 port를 받아옴.
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd)
{
  int is_static;
  int is_head = 0;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  //connfd 파일 디스크립터 즉, 연결이 되었다면, 연결된 소켓을 
  // rio함수를 사용하여 buf에 읽어온다.

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers : \n");
  printf("%s", buf);

  // 버퍼의 저장된 값을 각각 나눠서 저장해준다.
  // GET /godzilla.gif HTTP/1.1 이렇게 오면
  // GET | /godzilla.gif | HTTP/1.1 이런식으로 공백을 기준으로 끊어줌.
  sscanf(buf, "%s %s %s", method, uri, version); 
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) { // 대소문자 상관없이 문자열 비교하는 함수. (같으면 0반환)
    clienterror(fd, method, "501", "Not implemented",  // GET요청이 아니라면 501에러 핸들링
                "Tiny does not implement this method");
    return;
  }

  if (strcasecmp(method, "HEAD") == 0){
    is_head = 1;
  }
  
  read_requesthdrs(&rio); // 해당 리퀘스트에 대한 내용을 출력한다. 
  //여기서 나온 값들은 대부분 브라우저가 나에게 보낸 값들
  // 해당 로그 조회시 나의 크롬브라우저는 http 1.1 사용중.
  // 나는 1.0으로 respose

  is_static = parse_uri(uri, filename, cgiargs); // uri 분리
  // GET /cgi-bin/adder?num1=3&num2=5 HTTP/1.1 
  // -> uri = "/cgi-bin/adder?num1=3&num2=5" 
  // → filename = "./cgi-bin/adder" 
  // → cgiargs = "num1=3&num2=5"이런식으로 처리됨.
  // 정적콘텐츠면 1 반환, 동적 콘텐츠면 0 반환.



  if (stat(filename, &sbuf) < 0) { // 해당 파일이 존재하는지 확인하는 시스템콜
    // 성공 시 1 , 실패 시 0 반환
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file"); // 파일이 없다면 404에러 헨들링
    return;
  }
                                       
  if(is_static) { // 정적 콘텐츠이면
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      // 일반파일인지 확인, 파일의 소유자가 읽기 권한을 가졌는지 확인 
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
        return;
    }
    // .html, .jpg, .csss 같은 파일을 클라이언트에게 전송
    serve_static(fd, filename, sbuf.st_size, is_head); 
  }

  else {// 동적 콘텐츠라면 
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      // 일기 파일인지 확인, 실행권한을 가졌는지 확인 
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    // 동적 콘텐츠를 생성 후 그 출력을 클라이언트에게 전송
    //fork() + execve() + dup2() + wait() 등을 활용 
    serve_dynamic(fd, filename, cgiargs, is_head); 
  }
}

void clienterror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // 문자열 포멧팅을 사용하여 버퍼에 출력 결과를 저장 후
  // rio_writen을 사용하여 사용자에게 전송 
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){  //줄이 빈줄 일때까지 계속해서 반복
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }

  else{
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, int is_head)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length : %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if (!is_head){
    srcfd = Open(filename, O_RDONLY, 0);

    // * mmap 을 사용하여 파일 메모리를 매핑하는 방식 
    // * 속도 면에서 커널의 파일을 읽어오는 건 이게 더 빠름
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);

    // *말록을 통한 파일에 동적메모리를 할당하는 방식
    // char *buf_m = malloc(filesize);
    // Rio_readn(srcfd, buf_m, filesize);
    // Close(srcfd);
    // Rio_writen(fd, buf_m, filesize);
    // Free(buf_m);
  }
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4" );
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head)
{
  char buf[MAXLINE], *emptylist[] = { NULL };


  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (!is_head){
    if(Fork() == 0) {
      setenv("QUERY_STRING", cgiargs, 1);
      Dup2(fd, STDOUT_FILENO);
      Execve(filename, emptylist, environ);
    }
    Wait(NULL);
  }
}