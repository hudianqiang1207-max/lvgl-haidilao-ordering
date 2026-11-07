/**
 * @file payment.c
 * @brief 支付管理模块实现
 * @details 实现支付请求、状态查询、支付处理等功能
 */

#include "payment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* 内部数据结构 */
static payment_t *g_payments = NULL;      /* 支付记录数组 */
static int g_payment_count = 0;           /* 支付记录数量 */
static int g_payment_capacity = 0;        /* 数组容量 */

static payment_callback_t g_payment_callback = NULL;  /* 支付回调函数 */
static void *g_callback_user_data = NULL;              /* 回调用户数据 */

/* 互斥锁 */
static pthread_mutex_t g_payment_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 支付方式配置 */
static payment_method_t g_payment_methods[] = {
    {PAYMENT_METHOD_WECHAT, "微信支付", "wechat_icon", 1, 1},
    {PAYMENT_METHOD_ALIPAY, "支付宝", "alipay_icon", 1, 2},
    {PAYMENT_METHOD_CASH, "现金支付", "cash_icon", 1, 3},
    {PAYMENT_METHOD_MEMBER, "会员卡", "member_icon", 1, 4}
};
static int g_payment_method_count = sizeof(g_payment_methods) / sizeof(g_payment_methods[0]);

/* 内部函数声明 */
static int payment_expand_array(void);
static int payment_find_index(const char *payment_id);
static int payment_save_to_db(const payment_t *payment);
static int payment_load_from_db(void);
static char *generate_payment_id(void);
static int identify_payment_method(const char *payment_code);

/**
 * @brief 初始化支付管理模块
 * @return 0-成功，非0-失败
 */
int payment_init(void)
{
    pthread_mutex_lock(&g_payment_mutex);
    
    /* 初始化支付记录数组 */
    g_payment_capacity = 100;
    g_payments = (payment_t *)malloc(sizeof(payment_t) * g_payment_capacity);
    if (g_payments == NULL) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -1;
    }
    g_payment_count = 0;
    
    /* 从数据库加载支付记录 */
    payment_load_from_db();
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    printf("支付管理模块初始化成功，共加载 %d 条支付记录\n", g_payment_count);
    return 0;
}

/**
 * @brief 销毁支付管理模块
 */
void payment_destroy(void)
{
    pthread_mutex_lock(&g_payment_mutex);
    
    if (g_payments != NULL) {
        free(g_payments);
        g_payments = NULL;
    }
    g_payment_count = 0;
    g_payment_capacity = 0;
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    printf("支付管理模块已销毁\n");
}

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
                   const char *payment_code, payment_t *payment)
{
    if (order_id == NULL || payment == NULL || amount <= 0) {
        return -1;
    }
    
    /* 验证支付方式 */
    if (payment_method < PAYMENT_METHOD_WECHAT || payment_method > PAYMENT_METHOD_MEMBER) {
        return -2;
    }
    
    /* 验证支付码（微信/支付宝需要） */
    if ((payment_method == PAYMENT_METHOD_WECHAT || payment_method == PAYMENT_METHOD_ALIPAY) &&
        (payment_code == NULL || !payment_validate_code(payment_code, payment_method))) {
        return -3;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    /* 检查容量 */
    if (g_payment_count >= g_payment_capacity) {
        if (payment_expand_array() != 0) {
            pthread_mutex_unlock(&g_payment_mutex);
            return -1;
        }
    }
    
    /* 生成支付ID */
    char *payment_id = generate_payment_id();
    if (payment_id == NULL) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -1;
    }
    
    /* 创建支付记录 */
    payment_t *new_payment = &g_payments[g_payment_count];
    strncpy(new_payment->payment_id, payment_id, sizeof(new_payment->payment_id) - 1);
    strncpy(new_payment->order_id, order_id, sizeof(new_payment->order_id) - 1);
    new_payment->amount = amount;
    new_payment->payment_method = payment_method;
    new_payment->status = PAYMENT_STATUS_PENDING;
    new_payment->create_time = time(NULL);
    new_payment->expire_time = time(NULL) + 300;  /* 5分钟超时 */
    new_payment->third_party_id[0] = '\0';
    new_payment->error_code[0] = '\0';
    new_payment->error_msg[0] = '\0';
    
    if (payment_code != NULL) {
        strncpy(new_payment->payment_code, payment_code, sizeof(new_payment->payment_code) - 1);
    } else {
        new_payment->payment_code[0] = '\0';
    }
    
    g_payment_count++;
    
    /* 复制支付信息 */
    memcpy(payment, new_payment, sizeof(payment_t));
    
    /* 保存到数据库 */
    payment_save_to_db(new_payment);
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    free(payment_id);
    
    printf("发起支付请求: payment_id=%s, order_id=%s, amount=%.2f, method=%d\n",
           payment->payment_id, order_id, amount, payment_method);
    
    /* 这里应该调用第三方支付接口 */
    /* 暂时模拟支付成功 */
    /* 在实际应用中，这里需要调用微信/支付宝的支付API */
    
    return 0;
}

/**
 * @brief 查询支付状态
 * @param payment_id 支付ID
 * @param payment 支付信息输出
 * @return 0-成功，非0-失败
 */
int payment_query_status(const char *payment_id, payment_t *payment)
{
    if (payment_id == NULL || payment == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -2;  /* 支付记录不存在 */
    }
    
    /* 复制支付信息 */
    memcpy(payment, &g_payments[index], sizeof(payment_t));
    
    pthread_mutex_unlock(&g_payment_mutex);
    return 0;
}

/**
 * @brief 取消支付
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_cancel(const char *payment_id)
{
    if (payment_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -2;  /* 支付记录不存在 */
    }
    
    /* 检查支付状态 */
    if (g_payments[index].status != PAYMENT_STATUS_PENDING) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -3;  /* 支付状态不允许取消 */
    }
    
    /* 更新支付状态 */
    g_payments[index].status = PAYMENT_STATUS_CANCEL;
    strncpy(g_payments[index].error_msg, "用户取消支付", sizeof(g_payments[index].error_msg) - 1);
    
    /* 保存到数据库 */
    payment_save_to_db(&g_payments[index]);
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    printf("取消支付: payment_id=%s\n", payment_id);
    
    /* 调用回调函数 */
    if (g_payment_callback != NULL) {
        g_payment_callback(&g_payments[index], g_callback_user_data);
    }
    
    return 0;
}

/**
 * @brief 注册支付回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0-成功，非0-失败
 */
int payment_register_callback(payment_callback_t callback, void *user_data)
{
    g_payment_callback = callback;
    g_callback_user_data = user_data;
    
    printf("注册支付回调函数\n");
    return 0;
}

/**
 * @brief 处理支付超时
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_handle_timeout(const char *payment_id)
{
    if (payment_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -2;
    }
    
    /* 检查是否超时 */
    time_t now = time(NULL);
    if (now <= g_payments[index].expire_time) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -3;  /* 未超时 */
    }
    
    /* 更新支付状态 */
    g_payments[index].status = PAYMENT_STATUS_TIMEOUT;
    strncpy(g_payments[index].error_msg, "支付超时", sizeof(g_payments[index].error_msg) - 1);
    
    /* 保存到数据库 */
    payment_save_to_db(&g_payments[index]);
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    printf("支付超时: payment_id=%s\n", payment_id);
    
    /* 调用回调函数 */
    if (g_payment_callback != NULL) {
        g_payment_callback(&g_payments[index], g_callback_user_data);
    }
    
    return 0;
}

/**
 * @brief 重试支付
 * @param payment_id 支付ID
 * @return 0-成功，非0-失败
 */
int payment_retry(const char *payment_id)
{
    if (payment_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -2;
    }
    
    /* 检查支付状态 */
    if (g_payments[index].status != PAYMENT_STATUS_FAILED && 
        g_payments[index].status != PAYMENT_STATUS_TIMEOUT) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -3;  /* 状态不允许重试 */
    }
    
    /* 重置支付状态 */
    g_payments[index].status = PAYMENT_STATUS_PENDING;
    g_payments[index].error_code[0] = '\0';
    g_payments[index].error_msg[0] = '\0';
    g_payments[index].expire_time = time(NULL) + 300;  /* 重新设置超时时间 */
    
    /* 保存到数据库 */
    payment_save_to_db(&g_payments[index]);
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    printf("重试支付: payment_id=%s\n", payment_id);
    return 0;
}

/**
 * @brief 获取支付统计信息
 * @param total_payments 总支付数输出
 * @param success_payments 成功支付数输出
 * @param total_amount 总金额输出
 */
void payment_get_statistics(int *total_payments, int *success_payments, float *total_amount)
{
    int total = 0, success = 0;
    float amount = 0.0f;
    
    pthread_mutex_lock(&g_payment_mutex);
    
    total = g_payment_count;
    for (int i = 0; i < g_payment_count; i++) {
        if (g_payments[i].status == PAYMENT_STATUS_SUCCESS) {
            success++;
            amount += g_payments[i].amount;
        }
    }
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    if (total_payments != NULL) *total_payments = total;
    if (success_payments != NULL) *success_payments = success;
    if (total_amount != NULL) *total_amount = amount;
}

/**
 * @brief 清理过期支付记录
 * @return 清理的记录数
 */
int payment_cleanup_expired(void)
{
    int cleaned = 0;
    time_t now = time(NULL);
    
    pthread_mutex_lock(&g_payment_mutex);
    
    for (int i = g_payment_count - 1; i >= 0; i--) {
        if (g_payments[i].status == PAYMENT_STATUS_TIMEOUT && 
            (now - g_payments[i].expire_time) > 86400) {  /* 超过24小时 */
            /* 从数组中删除（移动后面的元素） */
            for (int j = i; j < g_payment_count - 1; j++) {
                memcpy(&g_payments[j], &g_payments[j + 1], sizeof(payment_t));
            }
            g_payment_count--;
            cleaned++;
        }
    }
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    if (cleaned > 0) {
        printf("清理过期支付记录: %d 条\n", cleaned);
    }
    
    return cleaned;
}

/**
 * @brief 检查支付码格式
 * @param payment_code 支付码
 * @param payment_method 支付方式
 * @return 1-有效，0-无效
 */
int payment_validate_code(const char *payment_code, int payment_method)
{
    if (payment_code == NULL || strlen(payment_code) == 0) {
        return 0;
    }
    
    int len = strlen(payment_code);
    
    switch (payment_method) {
        case PAYMENT_METHOD_WECHAT:
            /* 微信支付码：18位数字 */
            if (len == 18) {
                for (int i = 0; i < len; i++) {
                    if (payment_code[i] < '0' || payment_code[i] > '9') {
                        return 0;
                    }
                }
                return 1;
            }
            break;
            
        case PAYMENT_METHOD_ALIPAY:
            /* 支付宝支付码：16-24位数字 */
            if (len >= 16 && len <= 24) {
                for (int i = 0; i < len; i++) {
                    if (payment_code[i] < '0' || payment_code[i] > '9') {
                        return 0;
                    }
                }
                return 1;
            }
            break;
            
        case PAYMENT_METHOD_CASH:
            /* 现金支付不需要支付码 */
            return 1;
            
        case PAYMENT_METHOD_MEMBER:
            /* 会员卡号：6-20位数字 */
            if (len >= 6 && len <= 20) {
                for (int i = 0; i < len; i++) {
                    if (payment_code[i] < '0' || payment_code[i] > '9') {
                        return 0;
                    }
                }
                return 1;
            }
            break;
    }
    
    return 0;
}

/**
 * @brief 获取支付方式名称
 * @param payment_method 支付方式
 * @param name 名称输出缓冲区
 * @param size 缓冲区大小
 * @return 0-成功，非0-失败
 */
int payment_get_method_name(int payment_method, char *name, int size)
{
    if (name == NULL || size <= 0) {
        return -1;
    }
    
    for (int i = 0; i < g_payment_method_count; i++) {
        if (g_payment_methods[i].method_id == payment_method) {
            strncpy(name, g_payment_methods[i].name, size - 1);
            name[size - 1] = '\0';
            return 0;
        }
    }
    
    strncpy(name, "未知", size - 1);
    name[size - 1] = '\0';
    return 0;
}

/**
 * @brief 获取可用支付方式列表
 * @param methods 支付方式数组输出
 * @param max_count 数组最大容量
 * @return 实际支付方式数量
 */
int payment_get_available_methods(payment_method_t *methods, int max_count)
{
    if (methods == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    
    for (int i = 0; i < g_payment_method_count && count < max_count; i++) {
        if (g_payment_methods[i].enabled) {
            memcpy(&methods[count], &g_payment_methods[i], sizeof(payment_method_t));
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 检查支付是否可取消
 * @param payment_id 支付ID
 * @return 1-可取消，0-不可取消
 */
int payment_can_cancel(const char *payment_id)
{
    if (payment_id == NULL) {
        return 0;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return 0;
    }
    
    int can_cancel = (g_payments[index].status == PAYMENT_STATUS_PENDING) ? 1 : 0;
    
    pthread_mutex_unlock(&g_payment_mutex);
    
    return can_cancel;
}

/**
 * @brief 获取支付剩余时间
 * @param payment_id 支付ID
 * @param remaining_seconds 剩余秒数输出
 * @return 0-成功，非0-失败
 */
int payment_get_remaining_time(const char *payment_id, int *remaining_seconds)
{
    if (payment_id == NULL || remaining_seconds == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_payment_mutex);
    
    int index = payment_find_index(payment_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_payment_mutex);
        return -2;
    }
    
    time_t now = time(NULL);
    if (now >= g_payments[index].expire_time) {
        *remaining_seconds = 0;
    } else {
        *remaining_seconds = (int)(g_payments[index].expire_time - now);
    }
    
    pthread_mutex_unlock(&g_payment_mutex);
    return 0;
}

/* 内部函数实现 */

/**
 * @brief 扩展支付记录数组
 * @return 0-成功，非0-失败
 */
static int payment_expand_array(void)
{
    int new_capacity = g_payment_capacity * 2;
    payment_t *new_array = (payment_t *)realloc(g_payments, 
                                                sizeof(payment_t) * new_capacity);
    if (new_array == NULL) {
        return -1;
    }
    
    g_payments = new_array;
    g_payment_capacity = new_capacity;
    
    printf("支付记录数组扩展至 %d 个元素\n", new_capacity);
    return 0;
}

/**
 * @brief 查找支付记录在数组中的索引
 * @param payment_id 支付ID
 * @return 索引，-1表示未找到
 */
static int payment_find_index(const char *payment_id)
{
    for (int i = 0; i < g_payment_count; i++) {
        if (strcmp(g_payments[i].payment_id, payment_id) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 保存支付记录到数据库
 * @param payment 支付记录
 * @return 0-成功，非0-失败
 */
static int payment_save_to_db(const payment_t *payment)
{
    /* 这里应该实现数据库保存逻辑 */
    /* 暂时只打印日志 */
    printf("保存支付记录到数据库: payment_id=%s, amount=%.2f, status=%d\n",
           payment->payment_id, payment->amount, payment->status);
    return 0;
}

/**
 * @brief 从数据库加载支付记录
 * @return 0-成功，非0-失败
 */
static int payment_load_from_db(void)
{
    /* 这里应该实现从数据库加载支付记录的逻辑 */
    /* 暂时只打印日志 */
    printf("从数据库加载支付记录\n");
    return 0;
}

/**
 * @brief 生成支付ID
 * @return 支付ID字符串，需要调用者free
 */
static char *generate_payment_id(void)
{
    char *payment_id = (char *)malloc(64);
    if (payment_id == NULL) {
        return NULL;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    snprintf(payment_id, 64, "PAY_%04d%02d%02d_%02d%02d%02d_%d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             g_payment_count + 1);
    
    return payment_id;
}

/**
 * @brief 识别支付方式
 * @param payment_code 支付码
 * @return 支付方式，0表示无法识别
 */
static int identify_payment_method(const char *payment_code)
{
    if (payment_code == NULL) {
        return 0;
    }
    
    int len = strlen(payment_code);
    
    /* 微信支付码：18位数字，以10、11、12、13、14、15开头 */
    if (len == 18) {
        if (payment_code[0] == '1' && (payment_code[1] >= '0' && payment_code[1] <= '5')) {
            return PAYMENT_METHOD_WECHAT;
        }
    }
    
    /* 支付宝支付码：16-24位数字，以25、26、27、28、29、30开头 */
    if (len >= 16 && len <= 24) {
        if (payment_code[0] == '2' && (payment_code[1] >= '5' && payment_code[1] <= '9')) {
            return PAYMENT_METHOD_ALIPAY;
        }
        if (payment_code[0] == '3' && payment_code[1] == '0') {
            return PAYMENT_METHOD_ALIPAY;
        }
    }
    
    return 0;  /* 无法识别 */
}
