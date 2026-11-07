/**
 * @file haidilao_ui.h
 * @brief 海底捞点餐平板界面头文件
 * @details 声明界面相关函数和数据结构
 */

#ifndef __HAIDILAO_UI_H__
#define __HAIDILAO_UI_H__

#include "../lvgl/lvgl.h"

/* 常量定义 */
#define MAX_MENU_ITEMS 100
#define MAX_CART_ITEMS 50
#define MAX_CATEGORY_NAME 32
#define MAX_MENU_ITEM_NAME 64
#define MAX_MENU_ITEM_DESC 256

/* 菜品数据结构 */
typedef struct {
    int item_id;                        /* 菜品ID */
    char name[MAX_MENU_ITEM_NAME];      /* 菜品名称 */
    char description[MAX_MENU_ITEM_DESC]; /* 菜品描述 */
    char category[MAX_CATEGORY_NAME];   /* 菜品分类 */
    char image_url[256];                /* 菜品图片URL */
    char image_path[256];               /* 本地图片路径 */
    float price;                        /* 菜品价格 */
    float original_price;               /* 原价 */
    int sales_count;                    /* 销量 */
    float rating;                       /* 评分 */
    int category_id;                    /* 分类ID */
    int status;                         /* 菜品状态：0-下架，1-上架 */
} menu_item_t;

/* 购物车数据结构 */
typedef struct {
    int item_id;                        /* 菜品ID */
    char name[MAX_MENU_ITEM_NAME];      /* 菜品名称 */
    float price;                        /* 单价 */
    int quantity;                       /* 数量 */
    char remark[64];                    /* 备注 */
} cart_item_t;

/* 订单数据结构 */
typedef struct {
    char order_id[64];                  /* 订单ID */
    int table_id;                       /* 桌号 */
    int customer_count;                 /* 用餐人数 */
    float total_amount;                 /* 总金额 */
    float discount_amount;              /* 折扣金额 */
    float actual_amount;                /* 实际支付金额 */
    int status;                         /* 订单状态 */
    time_t create_time;                 /* 创建时间 */
    char remark[128];                   /* 订单备注 */
} order_t;

/* 函数声明 */

/**
 * @brief 初始化界面系统
 * @param font 中文字体指针
 */
void ui_init(lv_font_t *font);

/**
 * @brief 显示指定分类的菜品
 * @param category_id 分类ID，0表示全部
 */
void show_menu_items_by_category(int category_id);

/**
 * @brief 更新购物车显示
 */
void update_cart_display(void);

/**
 * @brief 显示菜品详情
 * @param item 菜品数据
 */
void show_menu_item_detail(const menu_item_t *item);

/**
 * @brief 添加菜品到购物车
 * @param item 菜品数据
 */
void add_item_to_cart(const menu_item_t *item);

/**
 * @brief 清空购物车
 */
void clear_cart(void);

/**
 * @brief 显示下单确认界面
 */
void show_checkout_confirmation(void);

/**
 * @brief 显示支付界面
 */
void show_payment_interface(void);

/**
 * @brief 显示成功提示
 * @param message 提示信息
 */
void show_success_prompt(const char *message);

/**
 * @brief 显示错误提示
 * @param message 提示信息
 */
void show_error_prompt(const char *message);

/**
 * @brief 更新状态栏时间
 * @param time_text 时间文本
 */
void update_status_bar_time(const char *time_text);

/**
 * @brief 更新网络状态
 * @param connected 网络连接状态：0-断开，1-连接
 */
void update_network_status(int connected);

/**
 * @brief 设置菜品数据
 * @param items 菜品数组
 * @param count 菜品数量
 */
void set_menu_items_data(const menu_item_t *items, int count);

/**
 * @brief 获取购物车总价
 * @return 总价
 */
float get_cart_total_price(void);

/**
 * @brief 获取购物车菜品数量
 * @return 菜品数量
 */
int get_cart_item_count(void);

/* 内部函数声明（静态函数） */
static void create_status_bar(lv_obj_t *parent);
static void create_category_nav(lv_obj_t *parent);
static void create_menu_grid(lv_obj_t *parent);
static void create_cart_button(lv_obj_t *parent);
static void create_cart_panel(lv_obj_t *parent);
static lv_obj_t *create_menu_item_card(lv_obj_t *parent, const menu_item_t *item);
static void load_menu_items_from_database(void);
static void category_btn_event_cb(lv_event_t *e);
static void menu_item_card_event_cb(lv_event_t *e);
static void add_to_cart_event_cb(lv_event_t *e);
static void cart_btn_event_cb(lv_event_t *e);
static void close_cart_panel_event_cb(lv_event_t *e);
static void checkout_btn_event_cb(lv_event_t *e);
static void add_to_cart_from_detail_event_cb(lv_event_t *e);
static void close_detail_dialog_event_cb(lv_event_t *e);
static void confirm_order_event_cb(lv_event_t *e);
static void cancel_order_event_cb(lv_event_t *e);
static void payment_method_btn_event_cb(lv_event_t *e);
static void create_order(void);
static void process_payment(int payment_method);
static void anim_ready_cb(lv_anim_t *anim);

#endif /* __HAIDILAO_UI_H__ */
