# 🌐 Web Proxy Lab - HTTP 프록시 서버 구현

> **네트워크 시스템 프로그래밍 프로젝트**: Echo 서버부터 멀티스레드 프록시 서버까지 단계적 구현

## 개요

이 프로젝트는 네트워크 프로그래밍의 핵심 개념을 학습하고, HTTP 프록시 서버를 단계적으로 구현한 과정을 담고 있습니다. 기본적인 Echo 서버부터 시작하여 Tiny 웹서버, 그리고 최종적으로 동시성과 캐싱 기능을 갖춘 프록시 서버까지 개발했습니다.

## 🏗️ 프로젝트 구조

```
webproxy_lab/
├── proxy.c              # 프록시 서버 메인 구현
├── tiny/                # Tiny 웹서버 구현
│   ├── tiny.c          # 웹서버 메인 로직
│   ├── cgi-bin/        # 동적 콘텐츠 스크립트
│   └── home.html       # 정적 콘텐츠
└── echo/               # Echo 서버 구현 (학습용)
```

## 📈 구현 단계

### 1. 네트워크 기초 개념 학습

**패킷화와 라우팅 이해**
- 데이터를 작은 패킷으로 분할하여 전송하는 패킷화 개념
- 최적 경로로 패킷을 전달하는 라우팅 메커니즘
- OSI 7계층 모델에서 네트워크 계층과 전송 계층 집중 학습

### 2. Echo Server 구현

**기본 TCP 소켓 프로그래밍**
```c
// 클라이언트 측 핵심 로직
clientfd = Open_clientfd(host, port);
Rio_readinitb(&rio, clientfd);

while(Fgets(buf, MAXLINE, stdin) != NULL) {
    Rio_writen(clientfd, buf, strlen(buf));
    Rio_readlineb(&rio, buf, MAXLINE);
    Fputs(buf, stdout);
}
```

**RIO 라이브러리 활용**
- 시스템 콜의 partial read/write 문제 해결
- 버퍼링을 통한 안정적인 I/O 처리
- 시그널 인터럽트 복구 메커니즘 구현

### 3. Tiny Web Server 구현

**HTTP 프로토콜 처리**
- HTTP 요청 파싱 (Method, URI, Version)
- 정적/동적 콘텐츠 구분 및 처리
- 적절한 HTTP 응답 헤더 생성

**주요 기능**
- GET, HEAD 메서드 지원
- MIME 타입별 정적 파일 서빙
- CGI를 통한 동적 콘텐츠 처리
- 에러 상황별 적절한 HTTP 상태 코드 반환

```c
void doit(int fd) {
    // HTTP 요청 라인 파싱
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    
    // URI 분석 및 파일 처리
    is_static = parse_uri(uri, filename, cgiargs);
    
    if (is_static) {
        serve_static(fd, filename, sbuf.st_size);
    } else {
        serve_dynamic(fd, filename, cgiargs);
    }
}
```

### 4. Proxy Server 구현

**프록시 서버의 핵심 기능**
- 클라이언트 요청을 받아 목적지 서버로 전달
- 서버 응답을 클라이언트에게 중계
- HTTP 헤더 재작성 및 최적화

**구현 특징**
```c
void doit(int clientfd) {
    // 클라이언트 요청 파싱
    parse_url(uri, hostname, path, port);
    
    // 목적지 서버와 연결
    proxyfd = Open_clientfd(hostname, port);
    
    // 요청 전달 및 응답 중계
    make_header(header_buf, hostname, path, user_agent_hdr);
    Rio_writen(proxyfd, header_buf, strlen(header_buf));
    
    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0) {
        Rio_writen(clientfd, buf, n);
    }
}
```

### 5. 동시성 지원 (멀티스레딩)

**pthread를 이용한 병렬 처리**
- 다중 클라이언트 동시 처리 구현
- 스레드별 독립적인 소켓 관리
- 스레드 리소스 관리 최적화

```c
// 메인 서버 루프
while(1) {
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    pthread_create(&tid, NULL, thread_func, connfdp);
}

// 스레드 함수
void *thread_func(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}
```

### 6. 캐싱 시스템 구현

**LRU 기반 캐시 관리**
- 자주 요청되는 콘텐츠를 메모리에 캐싱
- LRU (Least Recently Used) 알고리즘으로 캐시 교체
- 멀티스레드 환경에서의 동기화 처리

**캐시 구조체 설계**
```c
typedef struct {
    char uri[MAXLINE];
    char object[MAX_OBJECT_SIZE];
    int size;
    int LRU;
    int is_used;
    pthread_mutex_t lock;
} cache_block;

typedef struct {
    cache_block blocks[CACHE_CNT];
    pthread_mutex_t global_lock;
} Cache;
```

**동기화 메커니즘**
- 읽기 작업: 개별 블록 단위 뮤텍스
- 쓰기 작업: 전역 뮤텍스로 전체 캐시 보호
- 데드락 방지를 위한 락 순서 관리

## 🎯 주요 성과

### 기술적 구현
- **완전한 HTTP/1.0 프록시 서버** 구현
- **멀티스레드 동시성** 지원으로 다중 클라이언트 처리
- **LRU 캐싱 시스템**으로 성능 최적화
- **견고한 에러 처리** 및 메모리 관리

### 성능 최적화
- 캐시 히트율 향상을 통한 응답 시간 단축
- 메모리 매핑(mmap) 활용한 파일 I/O 최적화
- 스레드 풀 개념 적용 가능한 구조 설계

### 학습 성과
- **시스템 프로그래밍** 깊이 있는 이해
- **네트워크 프로토콜** 실제 구현 경험
- **동시성 프로그래밍**의 복잡성과 해결 방안 체득
- **성능 최적화** 기법들의 실전 적용

## 🛠️ 기술 스택

- **언어**: C
- **네트워킹**: Berkeley Sockets API
- **동시성**: POSIX Threads (pthread)
- **개발환경**: Docker, VSCode DevContainer
- **디버깅**: GDB를 활용한 멀티스레드 디버깅

## 📊 테스트 결과

카네기 멜론 대학의 자동 채점 시스템에서 **만점**을 달성했습니다:
- 기본 프록시 기능: 완벽 구현
- 동시성 처리: 다중 클라이언트 동시 처리 성공
- 캐싱 시스템: LRU 알고리즘 정상 동작 확인

## 🔍 핵심 기술 

### 1. 소켓 프로그래밍 숙련도
- TCP 연결 생성부터 종료까지 전체 라이프사이클 관리
- 비동기 I/O와 블로킹 I/O의 적절한 활용
- 네트워크 예외 상황 처리

### 2. HTTP 프로토콜 이해
- 요청/응답 메시지 파싱 및 생성
- 헤더 필드 처리 및 최적화
- 상태 코드별 적절한 응답 처리

### 3. 시스템 수준 최적화
- 메모리 매핑을 통한 효율적 파일 전송
- 버퍼링 전략으로 시스템 콜 오버헤드 최소화
- 멀티스레드 환경에서의 자원 관리


---

**참고 자료**: Computer Systems: A Programmer's Perspective (CS:APP)

