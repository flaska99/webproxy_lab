#include "csapp.h"

void echo(int confd);

int main(int argc, char **argv){
    int listenfd, connfd; // 서버 리슨 소켓 및 연결 소켓 (클라이언트마다 새로 생성)
    socklen_t clientlen; // 클라이언트 주소 구조체의 크기 (accept에 사용됨)

    // 클라이언트 주소를 저장할 구조체 (IPv4/IPv6 모두 호환 가능)
    // IPv6, IPv4 모두 호환하는 범용 구조체 사용
    struct sockaddr_storage clientaddr;

    // 클라이언트 호스트 이름, 포트 번호 저장용 문자열 버퍼
    char client_hostname [MAXLINE], client_port[MAXLINE];

    if(argc != 2) { // 명령행 인자는 프로그램명 + 포트번호여야 하므로 argc == 2 검사
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    // 지정된 포트 번호로 리슨 소켓을 열고, 연결 대기 상태로 진입
    listenfd = Open_listenfd(argv[1]); // 입력한 포트 번호로 소켓 구성

    while(1) {
        clientlen = sizeof(struct sockaddr_storage); // 초기화 (매번 필요)

        // 클라이언트가 연결 요청 → 수락하고 새로운 연결 소켓 반환
        // 타입 별칭 SA 사용해서 sockaddr 인자 넘기기
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 클라이언트 주소 정보를 호스트명 + 포트 문자열로 변환
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        // 클라이언트와 데이터 주고받기
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}