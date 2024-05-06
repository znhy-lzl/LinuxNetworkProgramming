#include <cerrno>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_EVENT_NUM  1024
#define BUFFER_SIZE 10

typedef void (*Callback)(epoll_event *event, int number, int epollfd, int listenfd);

int setnonblocking(int fd);
void addfd(int epollfd, int fd, bool enable_et);
void lt(epoll_event *events, int number, int epollfd, int listenfd);
void et(epoll_event *event, int number, int epollfd, int listenfd);

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create1(0);
    assert(epollfd != -1);

    /* 注册listenfd到epollfd */
    addfd(epollfd, listenfd, true);

    while (1) {
        int nready = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if (nready < 0) {
            printf("epoll failure\n");
            break;
        }

        /* 使用LT模式 */
        Callback cb = lt;

        /* 使用ET模式 */
        // Callback cb = et;

        cb(events, nready, epollfd, listenfd);
    }

    close(listenfd);
    return 0;

}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/*
将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中
enable_et指定是否对fd启用ET模式
*/
void addfd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/* LT 模式工作流程 */
void lt(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int fd = events[i].data.fd;
        if (fd == listenfd) { /* 新连接进来 */
            struct sockaddr_in client_addr;
            socklen_t client_addr_length = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_length);
            /* 对connfd禁用ET模式 */
            addfd(epollfd, connfd, false);
        } else if(events[i].events & EPOLLIN) {
            /* 只要缓冲区中还有未读的数据，就会一直触发 */
            printf("event trigger once\n");
            memset(buf, 0, BUFFER_SIZE);
            int recv_bytes = recv(fd, buf, BUFFER_SIZE - 1, 0);
            if (recv_bytes <= 0) {
                close(fd);
                continue;
            }
            printf("get %d bytes of content: %s\n", recv_bytes, buf);
        } else {
            printf("something else happened\n");
        }
    }
}

/* ET 模式工作流程 */
void et(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int fd = events[i].data.fd;
        if (fd == listenfd) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_length = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_length);
            /* 对connfd开启ET模式 */
            addfd(epollfd, connfd, true);
        } else if (events[i].events & EPOLLIN) {
            /* 这段代码不会被重复触发，所以我们循环读取数据，以确保把socket缓冲区中的所有数据读出 */
            printf("event trigger once\n");
            while (1) {
                memset(buf, 0, BUFFER_SIZE);
                int recv_bytes = recv(fd, buf, BUFFER_SIZE - 1, 0);
                if (recv_bytes < 0) {
                    /*
                        对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕。
                        此后，epoll就能再次触发fd上的EPOLLIN事件，以驱动下一次读操作
                    */
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read later\n");
                        break;
                    }
                    close(fd);
                    break;;
                } else if (recv_bytes == 0) {
                    close(fd);
                } else {
                    printf("get %d bytes of content: %s\n", recv_bytes, buf);
                }
            }
        } else {
            printf("something else happened\n");
        }
    }
}
