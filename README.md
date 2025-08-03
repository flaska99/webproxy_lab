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
- 시그널 인터럽트리

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

