#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    

    int sendbuf = atoi(argv[3]);
    int len = sizeof(sendbuf);
    /* 先设置 TCP 发送缓冲区的大小，然后立即读取 */
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf,sizeof(sendbuf));
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t *)&len);
    printf("the tcp send buffer size after setting is %d\n", sendbuf);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != -1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 'a', BUFFER_SIZE);
        send(sockfd, buffer, BUFFER_SIZE, 0);
    }

    close(sockfd);
    
    return 0;
}
