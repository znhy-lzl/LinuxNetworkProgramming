#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <string.h>
#include <cstdio>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static bool stop = false;

static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, handle_term);
    
    if (argc <= 3) {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    /* 创建一个 IPv4 socket 地址 */
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, backlog);
    assert(ret != -1);

    /* 暂停20s以等待客户端连接和相关操作（掉线或退出）完成 */
    sleep(60);
    struct sockaddr_in client;
    socklen_t client_addr_length = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addr_length);
    if (connfd < 0) {
        printf("error is: %s\n", strerror(errno));
    } else {
        /* 接受连接成功则打印出客户端的IP地址和端口号 */
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip: %s and port: %d\n",inet_ntop(AF_INET, &client.sin_addr, remote, 
          INET_ADDRSTRLEN), ntohs(client.sin_port));

        close(sockfd);
    }

    /* 关闭 socket */
    close(sockfd);

    return 0;
}
