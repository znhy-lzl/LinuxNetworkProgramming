#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int recvbuf = atoi(argv[4]);
    int len = sizeof(recvbuf);
    /* 先设置 TCP 接收缓冲区的大小，然后立即读取 */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t *)&len);
    printf("the tcp receive buffer size after setting is %d\n", recvbuf);

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, backlog);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addr_length = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addr_length);
    if (connfd < 0) {
        printf("error: %s", strerror(errno));
    } else {
        char buffer[BUF_SIZE];
        memset(buffer, '\0', BUF_SIZE);
        while (recv(connfd, buffer, BUF_SIZE - 1, 0) > 0);

        close(connfd);
    }

    close(sockfd);
    return 0;
}
