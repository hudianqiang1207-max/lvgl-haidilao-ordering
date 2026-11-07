/**
 * @file payment.h
 * @brief 支付管理模块头文件
 * @details 声明支付管理相关函数和数据结构
 */

#ifndef __PAYMENT_H__
#define __PAYMENT_H__

#include <time.h>

/* 支付方式定义 */
#define PAYMENT_METHOD_WECHAT    1   /* 微信支付 */
#define PAYMENT_METHOD_ALIPAY    2   /* 支付宝 */
#define PAYMENT_METHOD_CASH      3   /* 现金 */
#define PAYMENT_METHOD_MEMBER    4   /* 会员卡 */

/* 支付状态定义 */
#define PAYMENT_STATUS_PENDING   0   /* 待支付 */
#define PAYMENT_STATUS_SUCCESS   1   /* 支付成功 */
#define PAYMENT_STATUS_FAILED    2   /* 支付失败 */
#define PAYMENT_STATUS_TIMEOUT   3   /* 支付超时 */
#define PAYMENT_STATUS_CANCEL    4   /* 支付取消 */

/* 支付信息结构体 */
typedef struct {
    char payment_id[64];        /* 支付ID */
    char order_id[64];          /* 关联订单ID */
    float amount;               /* 支付金额 */
    int payment_method;         /* 支付方式 */
    char payment_code[128];     /* 支付码 */
    int status;                 /* 支付状态 */
    time_t create_time;         /* 创建时间 */
    time_t expire_time;         /* 过期时间 */
    char third_party_id[64];    /* 第三方支付ID */
    char error_code[32];        /* 错误代码 */
    char error_msg[128];        /* 错误信息 */
} payment_t;

/* 支付方式配置 */
typedef struct {
    int method_id;              /* 支付方式ID */
    char name[32];              /* 支付方式名称 */
    char icon[64];              /* 支付方式图标 */
    int enabled;                /* 是否启用 */
    int sort_order;             /* 排序顺序 */
} payment_method_t;

/* 支付回调函数类型 */
typedef void (*payment_callback_t)(const payment_t *payment, void *user_data);

/* 函数声明 */

/**
 * @brief 初始化支付管理模块
 * @return 0-成功，非0-失败
 */
int payment_init(void);

/**
 * @brief 销毁支付管理模块
 */
void payment_destroy(void);

/**
 * @brief 发起支付请求
 * @param order_id 订单ID
 * @param amount 支付金额
 * @param payment_method 支付方式
 * @param payment_code 支付码（微信/支付宝）
 * @param payment 支付信息输出
 * @return 0-成功，非0-失败
 */
int payment_request(const char *order_id, float amount, int payment_method, 
                   const char *payment_code, payment_t *payment);

/**
 * @brief 查询支付状态
 * @param payment_id 支付ID
 * @param payment 支付信息输出
 * @return 0-成功，非0-失败
 */
int payment_query_status(const char *payment_id, payment_t *payment);

/**
 * @brief 取消支付
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_cancel(const char *payment_id);

/**
 * @brief 注册支付回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0-成功，非0-失败
 */
int payment_register_callback(payment_callback_t callback, void *user_data);

/**
 * @brief 处理支付超时
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_handle_timeout(const char *payment_id);

/**
 * @brief 重试支付
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_retry(const char *payment_id);

/**
 * @brief 获取支付统计信息
 * @param total_payments 总支付数输出
 * @param success_payments 成功支付数输出
 * @param total_amount 总金额输出
 */
void payment_get_statistics(int *total_payments, int *success_payments, float *total_amount);

/**
 * @brief 清理过期支付记录
 * @return 清理的记录数
 */
int payment_cleanup_expired(void);

/**
 * @brief 检查支付码格式
 * @param payment_code 支付码
 * @param payment_method 支付方式
 * @return 1-有效，0-无效
 */
int payment_validate_code(const char *payment_code, int payment_method);

/**
 * @brief 获取支付方式名称
 * @param payment_method 支付方式
 * @param name 名称输出缓冲区
 * @param size 缓冲区大小
 * @return 0-成功，非0-失败
 */
int payment_get_method_name(int payment_method, char *name, int size);

/**
 * @brief 获取可用支付方式列表
 * @param methods 支付方式数组输出
 * @param max_count 数组最大容量
 * @return 实际支付方式数量
 */
int payment_get_available_methods(payment_method_t *methods, int max_count);

/**
 * @brief 检查支付是否可取消
 * @param payment_id 支付ID
 * @return 1-可取消，0-不可取消
 */
int payment_can_cancel(const char *payment_id);

/**
 * @brief 获取支付剩余时间
 * @param payment_id 支付ID
 * @param remaining_seconds 剩余秒数输出
 * @return 0-成功，非0-失败
 */
int payment_get_remaining_time(const char *payment_id, int *remaining_seconds);

#endif /* __PAYMENT_H__ */
