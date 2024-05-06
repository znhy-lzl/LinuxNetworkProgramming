#include <cassert>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/uio.h>

#define BUFFER_SIZE 1024

/* 定义两种HTTP状态码和状态信息 */
static const char *status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char *argv[])
{
    if (argc <= 4) {
        printf("usage: %s ip_address port_number backlog filename\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    /* 目标文件 */
    const char *filename = argv[4];

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
        /* 用于保存 HTTP 应答的状态行、头部字段和一个空行的缓冲区 */
        char header_buf[BUFFER_SIZE];
        memset(header_buf, 0, BUFFER_SIZE);
        /* 用于存放目标文件内容的应用程序缓存 */
        char *file_buf;
        /* 用于获取目标文件的属性，比如是否为目录，文件大小等 */
        struct stat file_stat;
        /* 记录目标文件是否是有效文件 */
        bool valid = true;
        /* 缓冲区header_buf目前已经使用了多少字节的空间 */
        int len = 0;

        if (stat(filename, &file_stat) < 0) { /* 目标文件不存在 */
            valid = false;
        } else {
            if (S_ISDIR(file_stat.st_mode)) { /* 目标文件是一个目录 */
                valid = false;
            } else if (file_stat.st_mode & S_IROTH) { /* 当前用户有读取目标文件的权限 */
                /* 动态分配缓存区file_buf, 并指定其大小为目标文件的大小file_stat.st_size 加 1
                 * 然后将目标文件读入缓存区 file_buf 中 
                 */
                int fd = open(filename, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, 0, file_stat.st_size + 1);
                if (read(fd, file_buf, file_stat.st_size) < 0) {
                    valid = false;
                }
            } else {
                valid = false;
            }
        }

        /* 如果目标文件有效，则发送正常的HTTP应答 */
        if(valid) {
            /*
             * 下面这部分内容将HTTP应答的状态行、“Content-Length” 头部字段和一个空行依次加入 header_buf 中
             */
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, 
                           "Content - Length: %ld\r\n", file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");

            /* 利用 writev 将 header_buf 和 file_buf 的内容一并写出 */
            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        } else {
            /* 如果目标文件无效，则通知客户端服务器发生了“内部错误” */
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buf, BUFFER_SIZE - 1 - len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }

        close(connfd);
        delete [] file_buf;

    }

    close(sockfd);
    return 0;
}
