/**
 * @file order.c
 * @brief 订单管理模块实现
 * @details 实现订单创建、查询、状态管理等功能
 */

#include "order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* 内部数据结构 */
static order_t *g_orders = NULL;           /* 订单数组 */
static int g_order_count = 0;              /* 订单数量 */
static int g_order_capacity = 0;           /* 数组容量 */

static order_item_t *g_order_items = NULL; /* 订单菜品数组 */
static int g_order_item_count = 0;         /* 订单菜品数量 */
static int g_order_item_capacity = 0;      /* 数组容量 */

static order_status_change_t *g_status_changes = NULL; /* 状态变更记录 */
static int g_status_change_count = 0;      /* 状态变更记录数量 */
static int g_status_change_capacity = 0;   /* 数组容量 */

/* 互斥锁 */
static pthread_mutex_t g_order_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_order_item_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_status_change_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 内部函数声明 */
static int order_find_index(const char *order_id);
static int order_expand_array(void);
static int order_item_expand_array(void);
static int status_change_expand_array(void);
static int order_save_to_db(const order_t *order);
static int order_load_from_db(void);
static int order_item_save_to_db(const order_item_t *item);
static int status_change_save_to_db(const order_status_change_t *change);
static char *generate_order_id(void);

/**
 * @brief 初始化订单管理模块
 * @return 0-成功，非0-失败
 */
int order_init(void)
{
    pthread_mutex_lock(&g_order_mutex);
    
    /* 初始化订单数组 */
    g_order_capacity = 100;
    g_orders = (order_t *)malloc(sizeof(order_t) * g_order_capacity);
    if (g_orders == NULL) {
        pthread_mutex_unlock(&g_order_mutex);
        return -1;
    }
    g_order_count = 0;
    
    /* 初始化订单菜品数组 */
    g_order_item_capacity = 500;
    g_order_items = (order_item_t *)malloc(sizeof(order_item_t) * g_order_item_capacity);
    if (g_order_items == NULL) {
        free(g_orders);
        g_orders = NULL;
        pthread_mutex_unlock(&g_order_mutex);
        return -1;
    }
    g_order_item_count = 0;
    
    /* 初始化状态变更记录数组 */
    g_status_change_capacity = 1000;
    g_status_changes = (order_status_change_t *)malloc(sizeof(order_status_change_t) * g_status_change_capacity);
    if (g_status_changes == NULL) {
        free(g_orders);
        free(g_order_items);
        g_orders = NULL;
        g_order_items = NULL;
        pthread_mutex_unlock(&g_order_mutex);
        return -1;
    }
    g_status_change_count = 0;
    
    /* 从数据库加载订单数据 */
    order_load_from_db();
    
    pthread_mutex_unlock(&g_order_mutex);
    
    printf("订单管理模块初始化成功，共加载 %d 条订单记录\n", g_order_count);
    return 0;
}

/**
 * @brief 销毁订单管理模块
 */
void order_destroy(void)
{
    pthread_mutex_lock(&g_order_mutex);
    
    if (g_orders != NULL) {
        free(g_orders);
        g_orders = NULL;
    }
    g_order_count = 0;
    g_order_capacity = 0;
    
    pthread_mutex_unlock(&g_order_mutex);
    
    pthread_mutex_lock(&g_order_item_mutex);
    
    if (g_order_items != NULL) {
        free(g_order_items);
        g_order_items = NULL;
    }
    g_order_item_count = 0;
    g_order_item_capacity = 0;
    
    pthread_mutex_unlock(&g_order_item_mutex);
    
    pthread_mutex_lock(&g_status_change_mutex);
    
    if (g_status_changes != NULL) {
        free(g_status_changes);
        g_status_changes = NULL;
    }
    g_status_change_count = 0;
    g_status_change_capacity = 0;
    
    pthread_mutex_unlock(&g_status_change_mutex);
    
    printf("订单管理模块已销毁\n");
}

/**
 * @brief 创建订单
 * @param table_id 桌号
 * @param customer_count 用餐人数
 * @param remark 订单备注
 * @param order 订单信息输出
 * @return 0-成功，非0-失败
 */
int order_create(int table_id, int customer_count, const char *remark, order_t *order)
{
    if (order == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    /* 检查容量 */
    if (g_order_count >= g_order_capacity) {
        if (order_expand_array() != 0) {
            pthread_mutex_unlock(&g_order_mutex);
            return -1;
        }
    }
    
    /* 生成订单ID */
    char *order_id = generate_order_id();
    if (order_id == NULL) {
        pthread_mutex_unlock(&g_order_mutex);
        return -1;
    }
    
    /* 创建订单 */
    order_t *new_order = &g_orders[g_order_count];
    strncpy(new_order->order_id, order_id, sizeof(new_order->order_id) - 1);
    new_order->table_id = table_id;
    new_order->customer_count = customer_count;
    new_order->total_amount = 0.0f;
    new_order->discount_amount = 0.0f;
    new_order->actual_amount = 0.0f;
    new_order->status = ORDER_STATUS_PENDING;
    new_order->create_time = time(NULL);
    new_order->pay_time = 0;
    new_order->complete_time = 0;
    new_order->machine_id[0] = '\0';
    
    if (remark != NULL) {
        strncpy(new_order->remark, remark, sizeof(new_order->remark) - 1);
    } else {
        new_order->remark[0] = '\0';
    }
    
    g_order_count++;
    
    /* 复制订单信息 */
    memcpy(order, new_order, sizeof(order_t));
    
    /* 保存到数据库 */
    order_save_to_db(new_order);
    
    /* 记录状态变更 */
    order_status_change_t change;
    change.change_id = g_status_change_count + 1;
    strncpy(change.order_id, order_id, sizeof(change.order_id) - 1);
    change.old_status = -1;  /* 初始状态 */
    change.new_status = ORDER_STATUS_PENDING;
    change.change_time = time(NULL);
    strncpy(change.operator, "系统", sizeof(change.operator) - 1);
    strncpy(change.remark, "创建订单", sizeof(change.remark) - 1);
    
    pthread_mutex_lock(&g_status_change_mutex);
    
    if (g_status_change_count >= g_status_change_capacity) {
        if (status_change_expand_array() != 0) {
            pthread_mutex_unlock(&g_status_change_mutex);
            pthread_mutex_unlock(&g_order_mutex);
            free(order_id);
            return -1;
        }
    }
    
    memcpy(&g_status_changes[g_status_change_count], &change, sizeof(order_status_change_t));
    g_status_change_count++;
    
    /* 保存状态变更到数据库 */
    status_change_save_to_db(&change);
    
    pthread_mutex_unlock(&g_status_change_mutex);
    
    pthread_mutex_unlock(&g_order_mutex);
    
    free(order_id);
    
    printf("创建订单成功: order_id=%s, table_id=%d, customer_count=%d\n",
           order->order_id, table_id, customer_count);
    
    return 0;
}

/**
 * @brief 添加订单菜品
 * @param order_id 订单ID
 * @param item_id 菜品ID
 * @param spec_id 规格ID
 * @param taste_id 口味ID
 * @param quantity 数量
 * @param unit_price 单价
 * @param remark 菜品备注
 * @return 0-成功，非0-失败
 */
int order_add_item(const char *order_id, int item_id, int spec_id, int taste_id,
                   int quantity, float unit_price, const char *remark)
{
    if (order_id == NULL || quantity <= 0 || unit_price <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    /* 检查订单是否存在 */
    int order_index = order_find_index(order_id);
    if (order_index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;  /* 订单不存在 */
    }
    
    /* 检查订单状态 */
    if (g_orders[order_index].status != ORDER_STATUS_PENDING) {
        pthread_mutex_unlock(&g_order_mutex);
        return -3;  /* 订单状态不允许添加菜品 */
    }
    
    pthread_mutex_unlock(&g_order_mutex);
    
    pthread_mutex_lock(&g_order_item_mutex);
    
    /* 检查容量 */
    if (g_order_item_count >= g_order_item_capacity) {
        if (order_item_expand_array() != 0) {
            pthread_mutex_unlock(&g_order_item_mutex);
            return -1;
        }
    }
    
    /* 添加订单菜品 */
    order_item_t *new_item = &g_order_items[g_order_item_count];
    new_item->order_item_id = g_order_item_count + 1;
    strncpy(new_item->order_id, order_id, sizeof(new_item->order_id) - 1);
    new_item->item_id = item_id;
    new_item->spec_id = spec_id;
    new_item->taste_id = taste_id;
    new_item->quantity = quantity;
    new_item->unit_price = unit_price;
    new_item->total_price = unit_price * quantity;
    
    if (remark != NULL) {
        strncpy(new_item->remark, remark, sizeof(new_item->remark) - 1);
    } else {
        new_item->remark[0] = '\0';
    }
    
    g_order_item_count++;
    
    /* 保存到数据库 */
    order_item_save_to_db(new_item);
    
    /* 更新订单总价 */
    pthread_mutex_lock(&g_order_mutex);
    
    order_index = order_find_index(order_id);
    if (order_index >= 0) {
        g_orders[order_index].total_amount += new_item->total_price;
        g_orders[order_index].actual_amount = g_orders[order_index].total_amount - 
                                              g_orders[order_index].discount_amount;
        order_save_to_db(&g_orders[order_index]);
    }
    
    pthread_mutex_unlock(&g_order_mutex);
    
    pthread_mutex_unlock(&g_order_item_mutex);
    
    printf("添加订单菜品成功: order_id=%s, item_id=%d, quantity=%d, total_price=%.2f\n",
           order_id, item_id, quantity, new_item->total_price);
    
    return 0;
}

/**
 * @brief 查询订单
 * @param order_id 订单ID
 * @param order 订单信息输出
 * @return 0-成功，非0-失败
 */
int order_query(const char *order_id, order_t *order)
{
    if (order_id == NULL || order == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;  /* 订单不存在 */
    }
    
    /* 复制订单信息 */
    memcpy(order, &g_orders[index], sizeof(order_t));
    
    pthread_mutex_unlock(&g_order_mutex);
    return 0;
}

/**
 * @brief 查询订单菜品
 * @param order_id 订单ID
 * @param items 订单菜品数组输出
 * @param max_count 数组最大容量
 * @return 实际菜品数量
 */
int order_query_items(const char *order_id, order_item_t *items, int max_count)
{
    if (order_id == NULL || items == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    
    pthread_mutex_lock(&g_order_item_mutex);
    
    for (int i = 0; i < g_order_item_count && count < max_count; i++) {
        if (strcmp(g_order_items[i].order_id, order_id) == 0) {
            memcpy(&items[count], &g_order_items[i], sizeof(order_item_t));
            count++;
        }
    }
    
    pthread_mutex_unlock(&g_order_item_mutex);
    
    return count;
}

/**
 * @brief 更新订单状态
 * @param order_id 订单ID
 * @param new_status 新状态
 * @param operator 操作员
 * @param remark 备注
 * @return 0-成功，非0-失败
 */
int order_update_status(const char *order_id, int new_status, const char *operator, const char *remark)
{
    if (order_id == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;  /* 订单不存在 */
    }
    
    /* 检查状态转换是否合法 */
    int old_status = g_orders[index].status;
    if (old_status == ORDER_STATUS_CANCELLED || old_status == ORDER_STATUS_COMPLETED) {
        pthread_mutex_unlock(&g_order_mutex);
        return -3;  /* 已取消或已完成的订单不能更新状态 */
    }
    
    /* 更新订单状态 */
    g_orders[index].status = new_status;
    
    /* 更新时间 */
    time_t now = time(NULL);
    if (new_status == ORDER_STATUS_PAID) {
        g_orders[index].pay_time = now;
    } else if (new_status == ORDER_STATUS_COMPLETED) {
        g_orders[index].complete_time = now;
    }
    
    /* 保存到数据库 */
    order_save_to_db(&g_orders[index]);
    
    /* 记录状态变更 */
    order_status_change_t change;
    change.change_id = g_status_change_count + 1;
    strncpy(change.order_id, order_id, sizeof(change.order_id) - 1);
    change.old_status = old_status;
    change.new_status = new_status;
    change.change_time = now;
    
    if (operator != NULL) {
        strncpy(change.operator, operator, sizeof(change.operator) - 1);
    } else {
        strncpy(change.operator, "系统", sizeof(change.operator) - 1);
    }
    
    if (remark != NULL) {
        strncpy(change.remark, remark, sizeof(change.remark) - 1);
    } else {
        change.remark[0] = '\0';
    }
    
    pthread_mutex_lock(&g_status_change_mutex);
    
    if (g_status_change_count >= g_status_change_capacity) {
        if (status_change_expand_array() != 0) {
            pthread_mutex_unlock(&g_status_change_mutex);
            pthread_mutex_unlock(&g_order_mutex);
            return -1;
        }
    }
    
    memcpy(&g_status_changes[g_status_change_count], &change, sizeof(order_status_change_t));
    g_status_change_count++;
    
    /* 保存状态变更到数据库 */
    status_change_save_to_db(&change);
    
    pthread_mutex_unlock(&g_status_change_mutex);
    
    pthread_mutex_unlock(&g_order_mutex);
    
    printf("更新订单状态成功: order_id=%s, %d -> %d\n", order_id, old_status, new_status);
    
    return 0;
}

/**
 * @brief 取消订单
 * @param order_id 订单ID
 * @param reason 取消原因
 * @return 0-成功，非0-失败
 */
int order_cancel(const char *order_id, const char *reason)
{
    if (order_id == NULL) {
        return -1;
    }
    
    /* 更新订单状态为已取消 */
    return order_update_status(order_id, ORDER_STATUS_CANCELLED, "用户", reason);
}

/**
 * @brief 获取订单状态变更历史
 * @param order_id 订单ID
 * @param changes 变更记录数组输出
 * @param max_count 数组最大容量
 * @return 实际记录数量
 */
int order_get_status_changes(const char *order_id, order_status_change_t *changes, int max_count)
{
    if (order_id == NULL || changes == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    
    pthread_mutex_lock(&g_status_change_mutex);
    
    /* 从最新记录开始查找 */
    for (int i = g_status_change_count - 1; i >= 0 && count < max_count; i--) {
        if (strcmp(g_status_changes[i].order_id, order_id) == 0) {
            memcpy(&changes[count], &g_status_changes[i], sizeof(order_status_change_t));
            count++;
        }
    }
    
    pthread_mutex_unlock(&g_status_change_mutex);
    
    return count;
}

/**
 * @brief 查询桌号当前订单
 * @param table_id 桌号
 * @param order 订单信息输出
 * @return 0-成功，非0-失败（无订单）
 */
int order_query_by_table(int table_id, order_t *order)
{
    if (order == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    /* 查找该桌号的最新未完成订单 */
    for (int i = g_order_count - 1; i >= 0; i--) {
        if (g_orders[i].table_id == table_id && 
            g_orders[i].status != ORDER_STATUS_COMPLETED &&
            g_orders[i].status != ORDER_STATUS_CANCELLED) {
            memcpy(order, &g_orders[i], sizeof(order_t));
            pthread_mutex_unlock(&g_order_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_order_mutex);
    return -2;  /* 无订单 */
}

/**
 * @brief 获取订单统计信息
 * @param total_orders 总订单数输出
 * @param pending_orders 待支付订单数输出
 * @param preparing_orders 制作中订单数输出
 * @param completed_orders 已完成订单数输出
 * @param total_amount 总金额输出
 */
void order_get_statistics(int *total_orders, int *pending_orders, 
                         int *preparing_orders, int *completed_orders, 
                         float *total_amount)
{
    int total = 0, pending = 0, preparing = 0, completed = 0;
    float amount = 0.0f;
    
    pthread_mutex_lock(&g_order_mutex);
    
    total = g_order_count;
    for (int i = 0; i < g_order_count; i++) {
        switch (g_orders[i].status) {
            case ORDER_STATUS_PENDING:
                pending++;
                break;
            case ORDER_STATUS_PREPARING:
                preparing++;
                break;
            case ORDER_STATUS_COMPLETED:
                completed++;
                break;
        }
        amount += g_orders[i].actual_amount;
    }
    
    pthread_mutex_unlock(&g_order_mutex);
    
    if (total_orders != NULL) *total_orders = total;
    if (pending_orders != NULL) *pending_orders = pending;
    if (preparing_orders != NULL) *preparing_orders = preparing;
    if (completed_orders != NULL) *completed_orders = completed;
    if (total_amount != NULL) *total_amount = amount;
}

/**
 * @brief 计算订单总价
 * @param order_id 订单ID
 * @param total_price 总价输出
 * @return 0-成功，非0-失败
 */
int order_calculate_total(const char *order_id, float *total_price)
{
    if (order_id == NULL || total_price == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;
    }
    
    *total_price = g_orders[index].total_amount;
    
    pthread_mutex_unlock(&g_order_mutex);
    return 0;
}

/**
 * @brief 应用优惠券
 * @param order_id 订单ID
 * @param coupon_code 优惠券码
 * @param discount_amount 折扣金额输出
 * @return 0-成功，非0-失败
 */
int order_apply_coupon(const char *order_id, const char *coupon_code, float *discount_amount)
{
    if (order_id == NULL || coupon_code == NULL || discount_amount == NULL) {
        return -1;
    }
    
    /* 这里应该查询优惠券信息 */
    /* 暂时返回固定折扣 */
    float discount = 10.0f;
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;
    }
    
    /* 应用折扣 */
    g_orders[index].discount_amount = discount;
    g_orders[index].actual_amount = g_orders[index].total_amount - discount;
    
    /* 保存到数据库 */
    order_save_to_db(&g_orders[index]);
    
    *discount_amount = discount;
    
    pthread_mutex_unlock(&g_order_mutex);
    
    printf("应用优惠券成功: order_id=%s, discount=%.2f\n", order_id, discount);
    return 0;
}

/**
 * @brief 检查订单是否可支付
 * @param order_id 订单ID
 * @return 1-可支付，0-不可支付
 */
int order_can_pay(const char *order_id)
{
    if (order_id == NULL) {
        return 0;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return 0;
    }
    
    int can_pay = (g_orders[index].status == ORDER_STATUS_PENDING) ? 1 : 0;
    
    pthread_mutex_unlock(&g_order_mutex);
    
    return can_pay;
}

/**
 * @brief 获取订单预计完成时间
 * @param order_id 订单ID
 * @param estimated_time 预计完成时间输出
 * @return 0-成功，非0-失败
 */
int order_get_estimated_time(const char *order_id, time_t *estimated_time)
{
    if (order_id == NULL || estimated_time == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&g_order_mutex);
    
    int index = order_find_index(order_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_order_mutex);
        return -2;
    }
    
    /* 根据订单状态计算预计完成时间 */
    time_t now = time(NULL);
    switch (g_orders[index].status) {
        case ORDER_STATUS_PENDING:
            *estimated_time = now + 1800;  /* 30分钟 */
            break;
        case ORDER_STATUS_PAID:
            *estimated_time = now + 1500;  /* 25分钟 */
            break;
        case ORDER_STATUS_PREPARING:
            *estimated_time = now + 900;   /* 15分钟 */
            break;
        case ORDER_STATUS_COMPLETED:
            *estimated_time = g_orders[index].complete_time;
            break;
        default:
            *estimated_time = 0;
            break;
    }
    
    pthread_mutex_unlock(&g_order_mutex);
    return 0;
}

/* 内部函数实现 */

/**
 * @brief 查找订单在数组中的索引
 * @param order_id 订单ID
 * @return 索引，-1表示未找到
 */
static int order_find_index(const char *order_id)
{
    for (int i = 0; i < g_order_count; i++) {
        if (strcmp(g_orders[i].order_id, order_id) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 扩展订单数组
 * @return 0-成功，非0-失败
 */
static int order_expand_array(void)
{
    int new_capacity = g_order_capacity * 2;
    order_t *new_array = (order_t *)realloc(g_orders, sizeof(order_t) * new_capacity);
    if (new_array == NULL) {
        return -1;
    }
    
    g_orders = new_array;
    g_order_capacity = new_capacity;
    
    printf("订单数组扩展至 %d 个元素\n", new_capacity);
    return 0;
}

/**
 * @brief 扩展订单菜品数组
 * @return 0-成功，非0-失败
 */
static int order_item_expand_array(void)
{
    int new_capacity = g_order_item_capacity * 2;
    order_item_t *new_array = (order_item_t *)realloc(g_order_items, 
                                                      sizeof(order_item_t) * new_capacity);
    if (new_array == NULL) {
        return -1;
    }
    
    g_order_items = new_array;
    g_order_item_capacity = new_capacity;
    
    printf("订单菜品数组扩展至 %d 个元素\n", new_capacity);
    return 0;
}

/**
 * @brief 扩展状态变更记录数组
 * @return 0-成功，非0-失败
 */
static int status_change_expand_array(void)
{
    int new_capacity = g_status_change_capacity * 2;
    order_status_change_t *new_array = (order_status_change_t *)realloc(g_status_changes, 
                                                                        sizeof(order_status_change_t) * new_capacity);
    if (new_array == NULL) {
        return -1;
    }
    
    g_status_changes = new_array;
    g_status_change_capacity = new_capacity;
    
    printf("状态变更记录数组扩展至 %d 个元素\n", new_capacity);
    return 0;
}

/**
 * @brief 保存订单到数据库
 * @param order 订单信息
 * @return 0-成功，非0-失败
 */
static int order_save_to_db(const order_t *order)
{
    /* 这里应该实现数据库保存逻辑 */
    /* 暂时只打印日志 */
    printf("保存订单到数据库: order_id=%s, status=%d, amount=%.2f\n",
           order->order_id, order->status, order->actual_amount);
    return 0;
}

/**
 * @brief 从数据库加载订单
 * @return 0-成功，非0-失败
 */
static int order_load_from_db(void)
{
    /* 这里应该实现从数据库加载订单的逻辑 */
    /* 暂时只打印日志 */
    printf("从数据库加载订单数据\n");
    return 0;
}

/**
 * @brief 保存订单菜品到数据库
 * @param item 订单菜品
 * @return 0-成功，非0-失败
 */
static int order_item_save_to_db(const order_item_t *item)
{
    /* 这里应该实现数据库保存逻辑 */
    /* 暂时只打印日志 */
    printf("保存订单菜品到数据库: order_id=%s, item_id=%d, quantity=%d\n",
           item->order_id, item->item_id, item->quantity);
    return 0;
}

/**
 * @brief 保存状态变更到数据库
 * @param change 状态变更记录
 * @return 0-成功，非0-失败
 */
static int status_change_save_to_db(const order_status_change_t *change)
{
    /* 这里应该实现数据库保存逻辑 */
    /* 暂时只打印日志 */
    printf("保存状态变更到数据库: order_id=%s, %d -> %d\n",
           change->order_id, change->old_status, change->new_status);
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
             g_order_count + 1);
    
    return order_id;
}
