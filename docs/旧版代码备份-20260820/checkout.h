/**
 * @file checkout.h
 * @brief 下单功能模块头文件
 * @details 声明下单相关函数和数据结构
 */

#ifndef __CHECKOUT_H__
#define __CHECKOUT_H__

#include "order.h"
#include "payment.h"

/* 下单状态定义 */
#define CHECKOUT_STATUS_IDLE        0   /* 空闲 */
#define CHECKOUT_STATUS_CONFIRMING  1   /* 确认中 */
#define CHECKOUT_STATUS_PAYING      2   /* 支付中 */
#define CHECKOUT_STATUS_SUCCESS     3   /* 下单成功 */
#define CHECKOUT_STATUS_FAILED      4   /* 下单失败 */

/* 下单信息结构体 */
typedef struct {
    char order_id[64];          /* 订单ID */
    int table_id;               /* 桌号 */
    int customer_count;         /* 用餐人数 */
    float total_amount;         /* 总金额 */
    float discount_amount;      /* 折扣金额 */
    float actual_amount;        /* 实际支付金额 */
    int payment_method;         /* 支付方式 */
    int status;                 /* 下单状态 */
    time_t create_time;         /* 创建时间 */
    char remark[128];           /* 备注 */
} checkout_info_t;

/* 下单回调函数类型 */
typedef void (*checkout_callback_t)(const checkout_info_t *info, void *user_data);

/* 函数声明 */

/**
 * @brief 初始化下单模块
 * @return 0-成功，非0-失败
 */
int checkout_init(void);

/**
 * @brief 销毁下单模块
 */
void checkout_destroy(void);

/**
 * @brief 开始下单流程
 * @param table_id 桌号
 * @param customer_count 用餐人数
 * @param payment_method 支付方式
 * @param remark 备注
 * @param info 下单信息输出
 * @return 0-成功，非0-失败
 */
int checkout_start(int table_id, int customer_count, int payment_method, 
                  const char *remark, checkout_info_t *info);

/**
 * @brief 确认下单
 * @param order_id 订单ID
 * @return 0-成功，非0-失败
 */
int checkout_confirm(const char *order_id);

/**
 * @brief 取消下单
 * @param order_id 订单ID
 * @return 0-成功，非0-失败
 */
int checkout_cancel(const char *order_id);

/**
 * @brief 获取下单状态
 * @param order_id 订单ID
 * @param info 下单信息输出
 * @return 0-成功，非0-失败
 */
int checkout_get_status(const char *order_id, checkout_info_t *info);

/**
 * @brief 注册下单回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0-成功，非0-失败
 */
int checkout_register_callback(checkout_callback_t callback, void *user_data);

/**
 * @brief 处理支付结果
 * @param payment 支付信息
 * @param user_data 用户数据
 */
void checkout_payment_callback(const payment_t *payment, void *user_data);

/**
 * @brief 获取下单统计信息
 * @param total_checkouts 总下单数输出
 * @param success_checkouts 成功下单数输出
 * @param total_amount 总金额输出
 */
void checkout_get_statistics(int *total_checkouts, int *success_checkouts, float *total_amount);

/**
 * @brief 清理过期下单记录
 * @return 清理的记录数
 */
int checkout_cleanup_expired(void);

/**
 * @brief 检查下单是否可取消
 * @param order_id 订单ID
 * @return 1-可取消，0-不可取消
 */
int checkout_can_cancel(const char *order_id);

/**
 * @brief 获取下单剩余时间
 * @param order_id 订单ID
 * @param remaining_seconds 剩余秒数输出
 * @return 0-成功，非0-失败
 */
int checkout_get_remaining_time(const char *order_id, int *remaining_seconds);

#endif /* __CHECKOUT_H__ */
