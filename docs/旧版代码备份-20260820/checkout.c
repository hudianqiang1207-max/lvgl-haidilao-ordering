/**
 * @file checkout.c
 * @brief 下单功能模块实现
 * @details 实现下单流程、状态管理、支付处理等功能
 */

#include "checkout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* 内部数据结构 */
static checkout_info_t *g_checkouts = NULL;  /* 下单记录数组 */
static int g_checkout_count = 0;             /* 下单记录数量 */
static int g_checkout_capacity = 0;          /* 数组容量 */

static checkout_callback_t g_checkout_callback = NULL;  /* 下单回调函数 */
static void *g_callback_user_data = NULL;                /* 回调用户数据 */

/* 互斥锁 */
static pthread_mutex_t g_checkout_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 内部函数声明 */
static int checkout_expand_array(void);
static int checkout_find_index(const char *order_id);
static int checkout_save_to_db(const checkout_info_t *info);
static int checkout_load_from_db(void);
static char *generate_order_id(void);

/**
 * @brief 初始化下单模块
 * @return 0-成功，非0-失败
 */
int checkout_init(void)
{
    pthread_mutex_lock(&g_checkout_mutex);
    
    /* 初始化下单记录数组 */
    g_checkout_capacity = 100;
    g_checkouts = (checkout_info_t *)malloc(sizeof(checkout_info_t) * g_checkout_capacity);
    if (g_checkouts == NULL) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -1;
    }
    g_checkout_count = 0;
    
    /* 从数据库加载下单记录 */
    checkout_load_from_db();
    
    /* 注册支付回调 */
    payment_register_callback(checkout_payment_callback, NULL);
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    printf("下单模块初始化成功，共加载 %d 条下单记录\n", g_checkout_count);
    return 0;
}

/**
 * @brief 销毁下单模块
 */
void checkout_destroy(void)
{
    pthread_mutex_lock(&g_checkout_mutex);
    
    if (g_checkouts != NULL) {
        free(g_checkouts);
        g_checkouts = NULL;
    }
    g_checkout_count = 0;
    g_checkout_capacity = 0;
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    printf("下单模块已销毁\n");
}

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
                  const char *remark, checkout_info_t *info)
{
    if (info == NULL) {
        return -1;
    }
    
    /* 检查桌号是否已有订单 */
    order_t existing_order;
    if (order_query_by_table(table_id, &existing_order) == 0) {
        if (existing_order.status != ORDER_STATUS_COMPLETED && 
            existing_order.status != ORDER_STATUS_CANCELLED) {
            return -2;  /* 桌号已有未完成订单 */
        }
    }
    
    /* 创建订单 */
    order_t order;
    if (order_create(table_id, customer_count, remark, &order) != 0) {
        return -3;  /* 创建订单失败 */
    }
    
    /* 计算订单总价 */
    float total_price;
    if (order_calculate_total(order.order_id, &total_price) != 0) {
        return -4;  /* 计算总价失败 */
    }
    
    /* 创建下单信息 */
    checkout_info_t *checkout_info = &g_checkouts[g_checkout_count];
    strncpy(checkout_info->order_id, order.order_id, sizeof(checkout_info->order_id) - 1);
    checkout_info->table_id = table_id;
    checkout_info->customer_count = customer_count;
    checkout_info->total_amount = total_price;
    checkout_info->discount_amount = 0.0f;
    checkout_info->actual_amount = total_price;
    checkout_info->payment_method = payment_method;
    checkout_info->status = CHECKOUT_STATUS_CONFIRMING;
    checkout_info->create_time = time(NULL);
    
    if (remark != NULL) {
        strncpy(checkout_info->remark, remark, sizeof(checkout_info->remark) - 1);
    } else {
        checkout_info->remark[0] = '\0';
    }
    
    g_checkout_count++;
    
    /* 复制下单信息 */
    memcpy(info, checkout_info, sizeof(checkout_info_t));
    
    /* 保存到数据库 */
    checkout_save_to_db(checkout_info);
    
    printf("开始下单流程: order_id=%s, table_id=%d, total=%.2f\n",
           order.order_id, table_id, total_price);
    
    return 0;
}

/**
 * @brief 确认下单
 * @param order_id 订单ID
 * @return 0-成功，非0-失败
 */
int checkout_confirm(const char *order_id)
{
    if (order_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -2;  /* 下单记录不存在 */
    }
    
    /* 检查状态 */
    if (g_checkouts[index].status != CHECKOUT_STATUS_CONFIRMING) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -3;  /* 状态不允许确认 */
    }
    
    /* 更新订单状态 */
    if (order_update_status(order_id, ORDER_STATUS_PAID, "系统", "用户确认下单") != 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -4;  /* 更新订单状态失败 */
    }
    
    /* 更新下单状态 */
    g_checkouts[index].status = CHECKOUT_STATUS_PAYING;
    
    /* 保存到数据库 */
    checkout_save_to_db(&g_checkouts[index]);
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    printf("确认下单: order_id=%s\n", order_id);
    
    /* 发起支付请求 */
    payment_t payment;
    if (payment_request(order_id, g_checkouts[index].actual_amount, 
                       g_checkouts[index].payment_method, NULL, &payment) != 0) {
        /* 支付请求失败，回滚订单状态 */
        order_update_status(order_id, ORDER_STATUS_PENDING, "系统", "支付请求失败");
        g_checkouts[index].status = CHECKOUT_STATUS_FAILED;
        checkout_save_to_db(&g_checkouts[index]);
        return -5;
    }
    
    return 0;
}

/**
 * @brief 取消下单
 * @param order_id 订单ID
 * @return 0-成功，非0-失败
 */
int checkout_cancel(const char *order_id)
{
    if (order_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -2;  /* 下单记录不存在 */
    }
    
    /* 检查是否可取消 */
    if (!checkout_can_cancel(order_id)) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -3;  /* 不可取消 */
    }
    
    /* 取消订单 */
    if (order_cancel(order_id, "用户取消") != 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -4;  /* 取消订单失败 */
    }
    
    /* 更新下单状态 */
    g_checkouts[index].status = CHECKOUT_STATUS_FAILED;
    
    /* 保存到数据库 */
    checkout_save_to_db(&g_checkouts[index]);
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    printf("取消下单: order_id=%s\n", order_id);
    
    /* 调用回调函数 */
    if (g_checkout_callback != NULL) {
        g_checkout_callback(&g_checkouts[index], g_callback_user_data);
    }
    
    return 0;
}

/**
 * @brief 获取下单状态
 * @param order_id 订单ID
 * @param info 下单信息输出
 * @return 0-成功，非0-失败
 */
int checkout_get_status(const char *order_id, checkout_info_t *info)
{
    if (order_id == NULL || info == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -2;  /* 下单记录不存在 */
    }
    
    /* 复制下单信息 */
    memcpy(info, &g_checkouts[index], sizeof(checkout_info_t));
    
    pthread_mutex_unlock(&g_checkout_mutex);
    return 0;
}

/**
 * @brief 注册下单回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0-成功，非0-失败
 */
int checkout_register_callback(checkout_callback_t callback, void *user_data)
{
    g_checkout_callback = callback;
    g_callback_user_data = user_data;
    
    printf("注册下单回调函数\n");
    return 0;
}

/**
 * @brief 处理支付结果
 * @param payment 支付信息
 * @param user_data 用户数据
 */
void checkout_payment_callback(const payment_t *payment, void *user_data)
{
    if (payment == NULL) {
        return;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(payment->order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return;
    }
    
    /* 根据支付结果更新下单状态 */
    switch (payment->status) {
        case PAYMENT_STATUS_SUCCESS:
            g_checkouts[index].status = CHECKOUT_STATUS_SUCCESS;
            /* 更新订单状态为已支付 */
            order_update_status(payment->order_id, ORDER_STATUS_PAID, "系统", "支付成功");
            printf("下单成功: order_id=%s\n", payment->order_id);
            break;
            
        case PAYMENT_STATUS_FAILED:
            g_checkouts[index].status = CHECKOUT_STATUS_FAILED;
            /* 回滚订单状态 */
            order_update_status(payment->order_id, ORDER_STATUS_PENDING, "系统", "支付失败");
            printf("下单失败: order_id=%s, error=%s\n", payment->order_id, payment->error_msg);
            break;
            
        case PAYMENT_STATUS_TIMEOUT:
            g_checkouts[index].status = CHECKOUT_STATUS_FAILED;
            /* 回滚订单状态 */
            order_update_status(payment->order_id, ORDER_STATUS_PENDING, "系统", "支付超时");
            printf("下单超时: order_id=%s\n", payment->order_id);
            break;
            
        case PAYMENT_STATUS_CANCEL:
            g_checkouts[index].status = CHECKOUT_STATUS_FAILED;
            /* 回滚订单状态 */
            order_update_status(payment->order_id, ORDER_STATUS_PENDING, "系统", "支付取消");
            printf("下单取消: order_id=%s\n", payment->order_id);
            break;
            
        default:
            break;
    }
    
    /* 保存到数据库 */
    checkout_save_to_db(&g_checkouts[index]);
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    /* 调用回调函数 */
    if (g_checkout_callback != NULL) {
        g_checkout_callback(&g_checkouts[index], g_callback_user_data);
    }
}

/**
 * @brief 获取下单统计信息
 * @param total_checkouts 总下单数输出
 * @param success_checkouts 成功下单数输出
 * @param total_amount 总金额输出
 */
void checkout_get_statistics(int *total_checkouts, int *success_checkouts, float *total_amount)
{
    int total = 0, success = 0;
    float amount = 0.0f;
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    total = g_checkout_count;
    for (int i = 0; i < g_checkout_count; i++) {
        if (g_checkouts[i].status == CHECKOUT_STATUS_SUCCESS) {
            success++;
            amount += g_checkouts[i].actual_amount;
        }
    }
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    if (total_checkouts != NULL) *total_checkouts = total;
    if (success_checkouts != NULL) *success_checkouts = success;
    if (total_amount != NULL) *total_amount = amount;
}

/**
 * @brief 清理过期下单记录
 * @return 清理的记录数
 */
int checkout_cleanup_expired(void)
{
    int cleaned = 0;
    time_t now = time(NULL);
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    for (int i = g_checkout_count - 1; i >= 0; i--) {
        if (g_checkouts[i].status == CHECKOUT_STATUS_FAILED && 
            (now - g_checkouts[i].create_time) > 86400) {  /* 超过24小时 */
            /* 从数组中删除（移动后面的元素） */
            for (int j = i; j < g_checkout_count - 1; j++) {
                memcpy(&g_checkouts[j], &g_checkouts[j + 1], sizeof(checkout_info_t));
            }
            g_checkout_count--;
            cleaned++;
        }
    }
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    if (cleaned > 0) {
        printf("清理过期下单记录: %d 条\n", cleaned);
    }
    
    return cleaned;
}

/**
 * @brief 检查下单是否可取消
 * @param order_id 订单ID
 * @return 1-可取消，0-不可取消
 */
int checkout_can_cancel(const char *order_id)
{
    if (order_id == NULL) {
        return 0;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return 0;
    }
    
    /* 只有确认中状态可取消 */
    int can_cancel = (g_checkouts[index].status == CHECKOUT_STATUS_CONFIRMING) ? 1 : 0;
    
    pthread_mutex_unlock(&g_checkout_mutex);
    
    return can_cancel;
}

/**
 * @brief 获取下单剩余时间
 * @param order_id 订单ID
 * @param remaining_seconds 剩余秒数输出
 * @return 0-成功，非0-失败
 */
int checkout_get_remaining_time(const char *order_id, int *remaining_seconds)
{
    if (order_id == NULL || remaining_seconds == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_checkout_mutex);
    
    int index = checkout_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_checkout_mutex);
        return -2;
    }
    
    /* 计算剩余时间（假设下单有效期30分钟） */
    time_t now = time(NULL);
    time_t expire_time = g_checkouts[index].create_time + 1800;  /* 30分钟 */
    
    if (now >= expire_time) {
        *remaining_seconds = 0;
    } else {
        *remaining_seconds = (int)(expire_time - now);
    }
    
    pthread_mutex_unlock(&g_checkout_mutex);
    return 0;
}

/* 内部函数实现 */

/**
 * @brief 扩展下单记录数组
 * @return 0-成功，非0-失败
 */
static int checkout_expand_array(void)
{
    int new_capacity = g_checkout_capacity * 2;
    checkout_info_t *new_array = (checkout_info_t *)realloc(g_checkouts, 
                                                            sizeof(checkout_info_t) * new_capacity);
    if (new_array == NULL) {
        return -1;
    }
    
    g_checkouts = new_array;
    g_checkout_capacity = new_capacity;
    
    printf("下单记录数组扩展至 %d 个元素\n", new_capacity);
    return 0;
}

/**
 * @brief 查找下单记录在数组中的索引
 * @param order_id 订单ID
 * @return 索引，-1表示未找到
 */
static int checkout_find_index(const char *order_id)
{
    for (int i = 0; i < g_checkout_count; i++) {
        if (strcmp(g_checkouts[i].order_id, order_id) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 保存下单记录到数据库
 * @param info 下单信息
 * @return 0-成功，非0-失败
 */
static int checkout_save_to_db(const checkout_info_t *info)
{
    /* 这里应该实现数据库保存逻辑 */
    /* 暂时只打印日志 */
    printf("保存下单记录到数据库: order_id=%s, status=%d, amount=%.2f\n",
           info->order_id, info->status, info->actual_amount);
    return 0;
}

/**
 * @brief 从数据库加载下单记录
 * @return 0-成功，非0-失败
 */
static int checkout_load_from_db(void)
{
    /* 这里应该实现从数据库加载下单记录的逻辑 */
    /* 暂时只打印日志 */
    printf("从数据库加载下单记录\n");
    return 0;
}

/**
 * @brief 生成订单ID
 * @return 订单ID字符串，需要调用者free
 */
static char *generate_order_id(void)
{
    char *order_id = (char *)malloc(64);
    if (order_id == NULL) {
        return NULL;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    snprintf(order_id, 64, "ORD_%04d%02d%02d_%02d%02d%02d_%d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             g_checkout_count + 1);
    
    return order_id;
}
