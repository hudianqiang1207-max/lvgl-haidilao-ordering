/**
 * @file order.h
 * @brief 订单管理模块头文件
 * @details 声明订单管理相关函数和数据结构
 */

#ifndef __ORDER_H__
#define __ORDER_H__

#include <time.h>

/* 订单状态定义 */
#define ORDER_STATUS_PENDING    0   /* 待支付 */
#define ORDER_STATUS_PAID       1   /* 已支付 */
#define ORDER_STATUS_PREPARING  2   /* 制作中 */
#define ORDER_STATUS_COMPLETED  3   /* 已完成 */
#define ORDER_STATUS_CANCELLED  4   /* 已取消 */

/* 订单数据结构 */
typedef struct {
    char order_id[64];          /* 订单ID */
    int table_id;               /* 桌号 */
    int customer_count;         /* 用餐人数 */
    float total_amount;         /* 总金额 */
    float discount_amount;      /* 折扣金额 */
    float actual_amount;        /* 实际支付金额 */
    int status;                 /* 订单状态 */
    time_t create_time;         /* 创建时间 */
    time_t pay_time;            /* 支付时间 */
    time_t complete_time;       /* 完成时间 */
    char remark[128];           /* 订单备注 */
    char machine_id[32];        /* 平板ID */
} order_t;

/* 订单菜品数据结构 */
typedef struct {
    int order_item_id;          /* 订单菜品ID */
    char order_id[64];          /* 关联订单ID */
    int item_id;                /* 菜品ID */
    int spec_id;                /* 规格ID */
    int taste_id;               /* 口味ID */
    int quantity;               /* 数量 */
    float unit_price;           /* 单价 */
    float total_price;          /* 总价 */
    char remark[64];            /* 菜品备注 */
} order_item_t;

/* 订单状态变更记录 */
typedef struct {
    int change_id;              /* 变更记录ID */
    char order_id[64];          /* 关联订单ID */
    int old_status;             /* 变更前状态 */
    int new_status;             /* 变更后状态 */
    time_t change_time;         /* 变更时间 */
    char operator[32];          /* 操作员 */
    char remark[128];           /* 备注 */
} order_status_change_t;

/* 函数声明 */

/**
 * @brief 初始化订单管理模块
 * @return 0-成功，非0-失败
 */
int order_init(void);

/**
 * @brief 销毁订单管理模块
 */
void order_destroy(void);

/**
 * @brief 创建订单
 * @param table_id 桌号
 * @param customer_count 用餐人数
 * @param remark 订单备注
 * @param order 订单信息输出
 * @return 0-成功，非0-失败
 */
int order_create(int table_id, int customer_count, const char *remark, order_t *order);

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
                   int quantity, float unit_price, const char *remark);

/**
 * @brief 查询订单
 * @param order_id 订单ID
 * @param order 订单信息输出
 * @return 0-成功，非0-失败
 */
int order_query(const char *order_id, order_t *order);

/**
 * @brief 查询订单菜品
 * @param order_id 订单ID
 * @param items 订单菜品数组输出
 * @param max_count 数组最大容量
 * @return 实际菜品数量
 */
int order_query_items(const char *order_id, order_item_t *items, int max_count);

/**
 * @brief 更新订单状态
 * @param order_id 订单ID
 * @param new_status 新状态
 * @param operator 操作员
 * @param remark 备注
 * @return 0-成功，非0-失败
 */
int order_update_status(const char *order_id, int new_status, const char *operator, const char *remark);

/**
 * @brief 取消订单
 * @param order_id 订单ID
 * @param reason 取消原因
 * @return 0-成功，非0-失败
 */
int order_cancel(const char *order_id, const char *reason);

/**
 * @brief 获取订单状态变更历史
 * @param order_id 订单ID
 * @param changes 变更记录数组输出
 * @param max_count 数组最大容量
 * @return 实际记录数量
 */
int order_get_status_changes(const char *order_id, order_status_change_t *changes, int max_count);

/**
 * @brief 查询桌号当前订单
 * @param table_id 桌号
 * @param order 订单信息输出
 * @return 0-成功，非0-失败（无订单）
 */
int order_query_by_table(int table_id, order_t *order);

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
                         float *total_amount);

/**
 * @brief 计算订单总价
 * @param order_id 订单ID
 * @param total_price 总价输出
 * @return 0-成功，非0-失败
 */
int order_calculate_total(const char *order_id, float *total_price);

/**
 * @brief 应用优惠券
 * @param order_id 订单ID
 * @param coupon_code 优惠券码
 * @param discount_amount 折扣金额输出
 * @return 0-成功，非0-失败
 */
int order_apply_coupon(const char *order_id, const char *coupon_code, float *discount_amount);

/**
 * @brief 检查订单是否可支付
 * @param order_id 订单ID
 * @return 1-可支付，0-不可支付
 */
int order_can_pay(const char *order_id);

/**
 * @brief 获取订单预计完成时间
 * @param order_id 订单ID
 * @param estimated_time 预计完成时间输出
 * @return 0-成功，非0-失败
 */
int order_get_estimated_time(const char *order_id, time_t *estimated_time);

#endif /* __ORDER_H__ */
