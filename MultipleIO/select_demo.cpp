#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    if (argc <= 3) {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int ret = 0;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    assert(ret != -1);

    ret = listen(listenfd, backlog);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_length);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
        close(listenfd);
    }

    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while (1) {
        memset(buf, '\0', sizeof(buf));

        /* 每次调用select前都要重新在read_fds和exception_fds中设置文件描述符connfd，因为在事件发生之后，文件描述符集合将被内核修改 */
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);

        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);
        if (ret < 0) {
            printf("select failure: %d\n", errno);
            break;
        }
        printf("ret : %d\n", ret);

        /* 对于可读事件，采用普通的recv函数读取数据 */
        if (FD_ISSET(connfd, &read_fds)) {
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);
        } else if (FD_ISSET(connfd, &exception_fds)) { /* 对于异常事件，采用带有MSG_OOB标志的recv函数读取带外数据 */
            printf("%d recv except data\n", connfd);
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of oob data: %s\n", ret, buf);
        }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
