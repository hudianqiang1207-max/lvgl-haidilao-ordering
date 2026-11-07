/**
 * @file net_client.c
 * @brief 海底捞点餐平板 - 客户端网络模块实现
 *
 * 开发板(ARM Linux)与 PC(SDL 仿真)共用此模块，
 * 通过 TCP 与 PC 服务器通信，所有命令均带超时保护，
 * 服务器不可用时返回错误，不影响界面离线运行。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "net_client.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCK(s) closesocket(s)
    #define SOCK_ERR INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #define CLOSE_SOCK(s) close(s)
    #define SOCK_ERR (-1)
    #define SOCKET int
#endif

#define CONNECT_TIMEOUT_MS 2000   /* 连接超时 */
#define RECV_LINE_MAX      1024

static SOCKET g_fd = SOCK_ERR;
static int g_connected = 0;
/* GET_MENU 的 62 条 ITEM 可能在一次 recv 中超过 2KB；过小会清空缓冲并丢行。 */
static char g_recv_buf[32768];
static int  g_recv_len = 0;

int net_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
#endif
    return 0;
}

/* 将 socket 设为非阻塞（用于带超时连接） */
static void set_nonblock(SOCKET fd)
{
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
#endif
}

int net_connect_server(void)
{
    if (g_connected) return 0;

    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == SOCK_ERR) return -1;
    set_nonblock(fd);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    /* 非阻塞 connect，用 select 等 2 秒 */
    int rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc != 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
            CLOSE_SOCK(fd);
            return -1;
        }
#else
        if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
            CLOSE_SOCK(fd);
            return -1;
        }
#endif
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        struct timeval tv = { CONNECT_TIMEOUT_MS / 1000,
                              (CONNECT_TIMEOUT_MS % 1000) * 1000 };
        int sel = select(fd + 1, NULL, &wfds, NULL, &tv);
        if (sel <= 0) {
            CLOSE_SOCK(fd);
            return -1;
        }
        int soerr = 0;
        socklen_t slen = sizeof(soerr);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&soerr, &slen);
        if (soerr != 0) {
            CLOSE_SOCK(fd);
            return -1;
        }
    }

    /* 恢复阻塞模式（后续用 select 做读超时） */
#ifdef _WIN32
    u_long mode = 0;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl & ~O_NONBLOCK);
#endif

    g_fd = fd;
    g_connected = 1;
    g_recv_len = 0;
    /* 必须确认并消费欢迎行，避免把 WELCOME 当成 MENU/ORDER 响应。 */
    {
        char welcome[128] = {0};
        if (net_recv_line(welcome, sizeof(welcome), 1500) != 0 ||
            strncmp(welcome, "WELCOME ", 8) != 0) {
            printf("[net] 欢迎行异常: %.100s\n", welcome);
            net_disconnect();
            return -1;
        }
    }
    printf("[net] 已连接服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    return 0;
}

void net_disconnect(void)
{
    if (g_fd != SOCK_ERR) CLOSE_SOCK(g_fd);
    g_fd = SOCK_ERR;
    g_connected = 0;
    g_recv_len = 0;
}

int net_is_connected(void)
{
    return g_connected;
}

int net_get_menu_version(unsigned long *version)
{
    char line[128];
    unsigned long v = 0;
    unsigned int v32 = 0;
    if (!version) return -1;
    if (net_send_line("MENU_VERSION") != 0) return -1;
    if (net_recv_line(line, sizeof(line), 1200) != 0) return -1;
    if (sscanf(line, "MENU_VERSION %u", &v32) != 1) return -1;
    v = (unsigned long)v32;
    *version = v;
    return 0;
}

/* 发送一行；若当前未连接则尝试自动重连一次（服务器可能中途重启） */
int net_send_line(const char *fmt, ...)
{
    if (!g_connected) {
        if (net_connect_server() != 0) return -1;   /* 自动重连 */
    }
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int n = (int)strlen(buf);
    buf[n++] = '\n';
    if (send(g_fd, buf, n, 0) != n) {
        net_disconnect();
        return -1;
    }
    return 0;
}

/* 等待可读（毫秒），返回 1 可读 / 0 超时 / -1 错误 */
static int wait_readable(int timeout_ms)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(g_fd, &rfds);
    struct timeval tv = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    return select(g_fd + 1, &rfds, NULL, NULL, &tv);
}

/* 从缓冲/网络取一行，去掉 \n，返回 0 成功 */
int net_recv_line(char *buf, int size, int timeout_ms)
{
    if (!g_connected || !buf || size <= 0) return -1;

    while (1) {
        /* 先在缓冲里找 \n */
        for (int i = 0; i < g_recv_len; i++) {
            if (g_recv_buf[i] == '\n') {
                int linelen = i;
                if (linelen >= size) linelen = size - 1;
                memcpy(buf, g_recv_buf, linelen);
                buf[linelen] = '\0';
                g_recv_len -= (i + 1);
                memmove(g_recv_buf, g_recv_buf + i + 1, g_recv_len);
                return 0;
            }
        }

        int wr = wait_readable(timeout_ms);
        if (wr <= 0) return -1;   /* 超时或出错 */
        char tmp[512];
        int n = recv(g_fd, tmp, sizeof(tmp), 0);
        if (n <= 0) {
            net_disconnect();
            return -1;
        }
        if (g_recv_len + n >= (int)sizeof(g_recv_buf)) {
            g_recv_len = 0;   /* 溢出丢弃 */
        }
        memcpy(g_recv_buf + g_recv_len, tmp, n);
        g_recv_len += n;
    }
}
