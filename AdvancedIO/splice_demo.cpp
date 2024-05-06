#include <cassert>
#include <cerrno>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);


    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    assert(ret != -1);

    ret = listen(sockfd, backlog);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addr_length = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addr_length);
    if (connfd < 0) {
        printf("error is: %s\n", strerror(errno));
    } else {
        int pipefd[2];
        assert(ret != - 1);
        ret = pipe(pipefd);   /* 创建管道 */
        /* 将 connfd 上流入的客户数据定向到管道中 */
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768,
            SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        close(connfd);
    }

    close(sockfd);

    return 0;
}
