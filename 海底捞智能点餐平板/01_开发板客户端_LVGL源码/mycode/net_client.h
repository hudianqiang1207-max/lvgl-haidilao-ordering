/**
 * @file net_client.h
 * @brief 海底捞点餐平板 - 客户端网络模块头文件
 *
 * 开发板作为客户端，通过 TCP 连接 PC 服务器端，
 * 提供菜单/库存拉取、订单提交、支付、出货等接口。
 */
#ifndef __NET_CLIENT_H__
#define __NET_CLIENT_H__

#include <stddef.h>

/* 服务器地址与端口（PC 端 IP，可按实际环境修改） */
#ifndef SERVER_IP
    #define SERVER_IP   "8.138.82.185"     /* 云服务器地址(阿里云公网) */
#endif
#ifndef SERVER_PORT
    #define SERVER_PORT 8888
#endif

/* 网络状态 */
typedef enum {
    NET_DISCONNECTED = 0,   /* 未连接 */
    NET_CONNECTED,          /* 已连接 */
    NET_TIMEOUT             /* 连接超时 */
} net_state_t;

/* 初始化网络（Windows/Linux 兼容），返回 0 成功 */
int  net_init(void);

/* 连接服务器（带超时，约 2 秒），返回 0 成功 */
int  net_connect_server(void);

/* 断开连接 */
void net_disconnect(void);

/* 当前是否已连接 */
int  net_is_connected(void);

/* 发送一行命令（自动补 \n），返回 0 成功 */
int  net_send_line(const char *fmt, ...);

/* 读取一行响应（去掉 \n），timeout_ms 为超时毫秒，返回 0 成功 */
int  net_recv_line(char *buf, int size, int timeout_ms);

/* 查询服务端菜单版本；不改变当前菜单数据 */
int  net_get_menu_version(unsigned long *version);

#endif /* __NET_CLIENT_H__ */
