/**
 * @file haidilao_ui.c
 * @brief 海底捞点餐平板界面实现
 * @details 实现菜单列表、分类展示、点餐、购物车等界面功能
 */

#include "haidilao_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 全局变量 */
static lv_obj_t *g_main_screen = NULL;      /* 主屏幕 */
static lv_obj_t *g_menu_grid = NULL;        /* 菜品网格 */
static lv_obj_t *g_category_nav = NULL;     /* 分类导航栏 */
static lv_obj_t *g_status_bar = NULL;       /* 状态栏 */
static lv_obj_t *g_cart_button = NULL;      /* 购物车按钮 */
static lv_obj_t *g_cart_panel = NULL;       /* 购物车面板 */

/* 菜品数据 */
static menu_item_t g_menu_items[MAX_MENU_ITEMS];
static int g_menu_item_count = 0;

/* 当前分类 */
static int g_current_category = 0;  /* 0表示全部分类 */

/* 购物车数据 */
static cart_item_t g_cart_items[MAX_CART_ITEMS];
static int g_cart_item_count = 0;

/* 字体 */
static lv_font_t *g_zh_font = NULL;

/**
 * @brief 初始化界面系统
 * @param font 中文字体指针
 */
void ui_init(lv_font_t *font)
{
    g_zh_font = font;
    
    /* 创建主屏幕 */
    g_main_screen = lv_obj_create(NULL);
    lv_scr_load(g_main_screen);
    
    /* 创建状态栏 */
    create_status_bar(g_main_screen);
    
    /* 创建分类导航栏 */
    create_category_nav(g_main_screen);
    
    /* 创建菜品网格 */
    create_menu_grid(g_main_screen);
    
    /* 创建购物车按钮 */
    create_cart_button(g_main_screen);
    
    /* 创建购物车面板（初始隐藏） */
    create_cart_panel(g_main_screen);
    
    /* 加载菜品数据 */
    load_menu_items_from_database();
    
    /* 显示菜品 */
    show_menu_items_by_category(g_current_category);
}

/**
 * @brief 创建状态栏
 * @param parent 父对象
 */
static void create_status_bar(lv_obj_t *parent)
{
    /* 状态栏容器 */
    g_status_bar = lv_obj_create(parent);
    lv_obj_set_size(g_status_bar, LV_PCT(100), 50);
    lv_obj_align(g_status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(g_status_bar, lv_color_hex(0xD32F2F), 0); /* 海底捞红色 */
    lv_obj_set_style_radius(g_status_bar, 0, 0);
    
    /* 餐厅名称 */
    lv_obj_t *restaurant_name = lv_label_create(g_status_bar);
    lv_label_set_text(restaurant_name, "海底捞火锅");
    lv_obj_set_style_text_font(restaurant_name, g_zh_font, 0);
    lv_obj_set_style_text_color(restaurant_name, lv_color_white(), 0);
    lv_obj_align(restaurant_name, LV_ALIGN_LEFT_MID, 15, 0);
    
    /* 桌号显示 */
    lv_obj_t *table_label = lv_label_create(g_status_bar);
    lv_label_set_text(table_label, "桌号: A08");
    lv_obj_set_style_text_font(table_label, g_zh_font, 0);
    lv_obj_set_style_text_color(table_label, lv_color_white(), 0);
    lv_obj_align(table_label, LV_ALIGN_CENTER, 0, 0);
    
    /* 当前时间 */
    lv_obj_t *time_label = lv_label_create(g_status_bar);
    lv_label_set_text(time_label, "12:00");
    lv_obj_set_style_text_font(time_label, g_zh_font, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -15, 0);
    
    /* 网络状态图标 */
    lv_obj_t *wifi_icon = lv_label_create(g_status_bar);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, lv_color_white(), 0);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -70, 0);
}

/**
 * @brief 创建分类导航栏
 * @param parent 父对象
 */
static void create_category_nav(lv_obj_t *parent)
{
    /* 分类导航栏容器 */
    g_category_nav = lv_obj_create(parent);
    lv_obj_set_size(g_category_nav, 120, LV_PCT(100) - 50);
    lv_obj_align(g_category_nav, LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_bg_color(g_category_nav, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_radius(g_category_nav, 0, 0);
    lv_obj_set_flex_flow(g_category_nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_category_nav, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(g_category_nav, 5, 0);
    
    /* 添加分类按钮 */
    const char *categories[] = {"全部", "锅底", "荤菜", "素菜", "主食", "饮品", "小吃", "甜点"};
    int category_count = sizeof(categories) / sizeof(categories[0]);
    
    for (int i = 0; i < category_count; i++) {
        lv_obj_t *btn = lv_btn_create(g_category_nav);
        lv_obj_set_size(btn, 110, 45);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
        lv_obj_set_style_radius(btn, 5, 0);
        lv_obj_set_style_margin_bottom(btn, 5, 0);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, categories[i]);
        lv_obj_set_style_text_font(label, g_zh_font, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);
        
        /* 设置按钮事件 */
        lv_obj_add_event_cb(btn, category_btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/**
 * @brief 创建菜品网格
 * @param parent 父对象
 */
static void create_menu_grid(lv_obj_t *parent)
{
    /* 菜品网格容器 */
    g_menu_grid = lv_obj_create(parent);
    lv_obj_set_size(g_menu_grid, LV_PCT(100) - 120, LV_PCT(100) - 100);
    lv_obj_align(g_menu_grid, LV_ALIGN_TOP_LEFT, 120, 50);
    lv_obj_set_style_bg_color(g_menu_grid, lv_color_white(), 0);
    lv_obj_set_style_radius(g_menu_grid, 0, 0);
    lv_obj_set_flex_flow(g_menu_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g_menu_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(g_menu_grid, 10, 0);
    lv_obj_set_style_pad_gap(g_menu_grid, 10, 0);
}

/**
 * @brief 创建购物车按钮
 * @param parent 父对象
 */
static void create_cart_button(lv_obj_t *parent)
{
    /* 购物车按钮 */
    g_cart_button = lv_btn_create(parent);
    lv_obj_set_size(g_cart_button, 60, 60);
    lv_obj_align(g_cart_button, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(g_cart_button, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_radius(g_cart_button, 30, 0);
    lv_obj_set_style_shadow_width(g_cart_button, 10, 0);
    lv_obj_set_style_shadow_color(g_cart_button, lv_color_hex(0x000000), 0);
    
    /* 购物车图标 */
    lv_obj_t *cart_icon = lv_label_create(g_cart_button);
    lv_label_set_text(cart_icon, LV_SYMBOL_CART);
    lv_obj_set_style_text_color(cart_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(cart_icon, g_zh_font, 0);
    lv_obj_center(cart_icon);
    
    /* 购物车数量标签 */
    lv_obj_t *count_label = lv_label_create(g_cart_button);
    lv_label_set_text(count_label, "0");
    lv_obj_set_style_text_color(count_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(count_label, g_zh_font, 0);
    lv_obj_align(count_label, LV_ALIGN_TOP_RIGHT, 5, -5);
    
    /* 设置按钮事件 */
    lv_obj_add_event_cb(g_cart_button, cart_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief 创建购物车面板
 * @param parent 父对象
 */
static void create_cart_panel(lv_obj_t *parent)
{
    /* 购物车面板（初始隐藏） */
    g_cart_panel = lv_obj_create(parent);
    lv_obj_set_size(g_cart_panel, 300, LV_PCT(100));
    lv_obj_align(g_cart_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(g_cart_panel, lv_color_white(), 0);
    lv_obj_set_style_radius(g_cart_panel, 0, 0);
    lv_obj_set_style_border_width(g_cart_panel, 2, 0);
    lv_obj_set_style_border_color(g_cart_panel, lv_color_hex(0xE0E0E0), 0);
    lv_obj_add_flag(g_cart_panel, LV_OBJ_FLAG_HIDDEN);
    
    /* 购物车标题 */
    lv_obj_t *title = lv_label_create(g_cart_panel);
    lv_label_set_text(title, "购物车");
    lv_obj_set_style_text_font(title, g_zh_font, 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    /* 关闭按钮 */
    lv_obj_t *close_btn = lv_btn_create(g_cart_panel);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF5722), 0);
    lv_obj_set_style_radius(close_btn, 15, 0);
    
    lv_obj_t *close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_icon, lv_color_white(), 0);
    lv_obj_center(close_icon);
    
    /* 关闭按钮事件 */
    lv_obj_add_event_cb(close_btn, close_cart_panel_event_cb, LV_EVENT_CLICKED, NULL);
    
    /* 购物车列表区域 */
    lv_obj_t *cart_list = lv_obj_create(g_cart_panel);
    lv_obj_set_size(cart_list, LV_PCT(100), LV_PCT(100) - 150);
    lv_obj_align(cart_list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(cart_list, lv_color_hex(0xFAFAFA), 0);
    lv_obj_set_style_radius(cart_list, 0, 0);
    
    /* 总价显示区域 */
    lv_obj_t *total_panel = lv_obj_create(g_cart_panel);
    lv_obj_set_size(total_panel, LV_PCT(100), 100);
    lv_obj_align(total_panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(total_panel, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_radius(total_panel, 0, 0);
    
    /* 总价标签 */
    lv_obj_t *total_label = lv_label_create(total_panel);
    lv_label_set_text(total_label, "总价: ¥0.00");
    lv_obj_set_style_text_font(total_label, g_zh_font, 0);
    lv_obj_set_style_text_color(total_label, lv_color_black(), 0);
    lv_obj_align(total_label, LV_ALIGN_TOP_MID, 0, 10);
    
    /* 下单按钮 */
    lv_obj_t *order_btn = lv_btn_create(total_panel);
    lv_obj_set_size(order_btn, LV_PCT(80), 50);
    lv_obj_align(order_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(order_btn, lv_color_hex(0xD32F2F), 0);
    lv_obj_set_style_radius(order_btn, 25, 0);
    
    lv_obj_t *order_label = lv_label_create(order_btn);
    lv_label_set_text(order_label, "去下单");
    lv_obj_set_style_text_font(order_label, g_zh_font, 0);
    lv_obj_set_style_text_color(order_label, lv_color_white(), 0);
    lv_obj_center(order_label);
    
    /* 下单按钮事件 */
    lv_obj_add_event_cb(order_btn, checkout_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief 创建菜品卡片
 * @param parent 父对象
 * @param item 菜品数据
 * @return 菜品卡片对象
 */
static lv_obj_t *create_menu_item_card(lv_obj_t *parent, const menu_item_t *item)
{
    /* 卡片容器 */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 180, 220);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_shadow_width(card, 5, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(card, 5, 0);
    
    /* 菜品图片 */
    lv_obj_t *img = lv_img_create(card);
    /* 这里应该加载菜品图片，暂时用占位符 */
    lv_obj_set_size(img, 170, 120);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(img, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_radius(img, 5, 0);
    
    /* 菜品名称 */
    lv_obj_t *name_label = lv_label_create(card);
    lv_label_set_text(name_label, item->name);
    lv_obj_set_style_text_font(name_label, g_zh_font, 0);
    lv_obj_set_style_text_color(name_label, lv_color_black(), 0);
    lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 130);
    
    /* 菜品价格 */
    lv_obj_t *price_label = lv_label_create(card);
    char price_text[32];
    snprintf(price_text, sizeof(price_text), "¥%.2f", item->price);
    lv_label_set_text(price_label, price_text);
    lv_obj_set_style_text_font(price_label, g_zh_font, 0);
    lv_obj_set_style_text_color(price_label, lv_color_hex(0xFF5722), 0);
    lv_obj_align(price_label, LV_ALIGN_TOP_MID, 0, 155);
    
    /* 销量显示 */
    lv_obj_t *sales_label = lv_label_create(card);
    char sales_text[32];
    snprintf(sales_text, sizeof(sales_text), "月售%d", item->sales_count);
    lv_label_set_text(sales_label, sales_text);
    lv_obj_set_style_text_font(sales_label, g_zh_font, 0);
    lv_obj_set_style_text_color(sales_label, lv_color_hex(0x9E9E9E), 0);
    lv_obj_align(sales_label, LV_ALIGN_TOP_MID, 0, 175);
    
    /* 添加按钮 */
    lv_obj_t *add_btn = lv_btn_create(card);
    lv_obj_set_size(add_btn, 30, 30);
    lv_obj_align(add_btn, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(add_btn, 15, 0);
    
    lv_obj_t *add_icon = lv_label_create(add_btn);
    lv_label_set_text(add_icon, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(add_icon, lv_color_white(), 0);
    lv_obj_center(add_icon);
    
    /* 添加按钮事件 */
    lv_obj_add_event_cb(add_btn, add_to_cart_event_cb, LV_EVENT_CLICKED, (void *)item);
    
    /* 设置卡片点击事件 */
    lv_obj_add_event_cb(card, menu_item_card_event_cb, LV_EVENT_CLICKED, (void *)item);
    
    return card;
}

/**
 * @brief 显示指定分类的菜品
 * @param category_id 分类ID，0表示全部
 */
void show_menu_items_by_category(int category_id)
{
    /* 清空菜品网格 */
    lv_obj_clean(g_menu_grid);
    
    /* 根据分类筛选菜品 */
    for (int i = 0; i < g_menu_item_count; i++) {
        if (category_id == 0 || g_menu_items[i].category_id == category_id) {
            create_menu_item_card(g_menu_grid, &g_menu_items[i]);
        }
    }
}

/**
 * @brief 从数据库加载菜品数据
 */
static void load_menu_items_from_database(void)
{
    /* 这里应该从数据库加载菜品数据 */
    /* 暂时使用示例数据 */
    g_menu_item_count = 8;
    
    /* 示例菜品1 */
    strcpy(g_menu_items[0].name, "番茄锅底");
    strcpy(g_menu_items[0].category, "锅底");
    g_menu_items[0].price = 38.00;
    g_menu_items[0].sales_count = 1256;
    g_menu_items[0].category_id = 1;
    
    /* 示例菜品2 */
    strcpy(g_menu_items[1].name, "麻辣锅底");
    strcpy(g_menu_items[1].category, "锅底");
    g_menu_items[1].price = 42.00;
    g_menu_items[1].sales_count = 2345;
    g_menu_items[1].category_id = 1;
    
    /* 示例菜品3 */
    strcpy(g_menu_items[2].name, "精品肥牛");
    strcpy(g_menu_items[2].category, "荤菜");
    g_menu_items[2].price = 58.00;
    g_menu_items[2].sales_count = 1890;
    g_menu_items[2].category_id = 2;
    
    /* 示例菜品4 */
    strcpy(g_menu_items[3].name, "虾滑");
    strcpy(g_menu_items[3].category, "荤菜");
    g_menu_items[3].price = 46.00;
    g_menu_items[3].sales_count = 1567;
    g_menu_items[3].category_id = 2;
    
    /* 示例菜品5 */
    strcpy(g_menu_items[4].name, "生菜");
    strcpy(g_menu_items[4].category, "素菜");
    g_menu_items[4].price = 12.00;
    g_menu_items[4].sales_count = 987;
    g_menu_items[4].category_id = 3;
    
    /* 示例菜品6 */
    strcpy(g_menu_items[5].name, "土豆片");
    strcpy(g_menu_items[5].category, "素菜");
    g_menu_items[5].price = 10.00;
    g_menu_items[5].sales_count = 1234;
    g_menu_items[5].category_id = 3;
    
    /* 示例菜品7 */
    strcpy(g_menu_items[6].name, "手擀面");
    strcpy(g_menu_items[6].category, "主食");
    g_menu_items[6].price = 8.00;
    g_menu_items[6].sales_count = 876;
    g_menu_items[6].category_id = 4;
    
    /* 示例菜品8 */
    strcpy(g_menu_items[7].name, "酸梅汤");
    strcpy(g_menu_items[7].category, "饮品");
    g_menu_items[7].price = 15.00;
    g_menu_items[7].sales_count = 2100;
    g_menu_items[7].category_id = 5;
}

/**
 * @brief 分类按钮事件回调
 * @param e 事件对象
 */
static void category_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int category_id = (int)(intptr_t)lv_event_get_user_data(e);
        g_current_category = category_id;
        show_menu_items_by_category(category_id);
    }
}

/**
 * @brief 菜品卡片事件回调
 * @param e 事件对象
 */
static void menu_item_card_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        menu_item_t *item = (menu_item_t *)lv_event_get_user_data(e);
        show_menu_item_detail(item);
    }
}

/**
 * @brief 添加到购物车事件回调
 * @param e 事件对象
 */
static void add_to_cart_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        menu_item_t *item = (menu_item_t *)lv_event_get_user_data(e);
        add_item_to_cart(item);
    }
}

/**
 * @brief 购物车按钮事件回调
 * @param e 事件对象
 */
static void cart_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* 显示购物车面板 */
        lv_obj_clear_flag(g_cart_panel, LV_OBJ_FLAG_HIDDEN);
        update_cart_display();
    }
}

/**
 * @brief 关闭购物车面板事件回调
 * @param e 事件对象
 */
static void close_cart_panel_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* 隐藏购物车面板 */
        lv_obj_add_flag(g_cart_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 下单按钮事件回调
 * @param e 事件对象
 */
static void checkout_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* 显示下单确认界面 */
        show_checkout_confirmation();
    }
}

/**
 * @brief 显示菜品详情
 * @param item 菜品数据
 */
static void show_menu_item_detail(const menu_item_t *item)
{
    /* 创建详情对话框 */
    lv_obj_t *dialog = lv_obj_create(g_main_screen);
    lv_obj_set_size(dialog, LV_PCT(80), LV_PCT(70));
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, lv_color_white(), 0);
    lv_obj_set_style_radius(dialog, 15, 0);
    lv_obj_set_style_shadow_width(dialog, 20, 0);
    
    /* 菜品图片 */
    lv_obj_t *img = lv_img_create(dialog);
    lv_obj_set_size(img, LV_PCT(90), 200);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(img, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_radius(img, 10, 0);
    
    /* 菜品名称 */
    lv_obj_t *name_label = lv_label_create(dialog);
    lv_label_set_text(name_label, item->name);
    lv_obj_set_style_text_font(name_label, g_zh_font, 0);
    lv_obj_set_style_text_color(name_label, lv_color_black(), 0);
    lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 230);
    
    /* 菜品价格 */
    lv_obj_t *price_label = lv_label_create(dialog);
    char price_text[32];
    snprintf(price_text, sizeof(price_text), "价格: ¥%.2f", item->price);
    lv_label_set_text(price_label, price_text);
    lv_obj_set_style_text_font(price_label, g_zh_font, 0);
    lv_obj_set_style_text_color(price_label, lv_color_hex(0xFF5722), 0);
    lv_obj_align(price_label, LV_ALIGN_TOP_MID, 0, 260);
    
    /* 菜品描述 */
    lv_obj_t *desc_label = lv_label_create(dialog);
    lv_label_set_text(desc_label, "菜品描述：暂无");
    lv_obj_set_style_text_font(desc_label, g_zh_font, 0);
    lv_obj_set_style_text_color(desc_label, lv_color_hex(0x757575), 0);
    lv_obj_align(desc_label, LV_ALIGN_TOP_MID, 0, 290);
    
    /* 添加到购物车按钮 */
    lv_obj_t *add_btn = lv_btn_create(dialog);
    lv_obj_set_size(add_btn, LV_PCT(60), 50);
    lv_obj_align(add_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(add_btn, 25, 0);
    
    lv_obj_t *add_label = lv_label_create(add_btn);
    lv_label_set_text(add_label, "加入购物车");
    lv_obj_set_style_text_font(add_label, g_zh_font, 0);
    lv_obj_set_style_text_color(add_label, lv_color_white(), 0);
    lv_obj_center(add_label);
    
    /* 添加按钮事件 */
    lv_obj_add_event_cb(add_btn, add_to_cart_from_detail_event_cb, LV_EVENT_CLICKED, (void *)item);
    
    /* 关闭按钮 */
    lv_obj_t *close_btn = lv_btn_create(dialog);
    lv_obj_set_size(close_btn, 40, 40);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF5722), 0);
    lv_obj_set_style_radius(close_btn, 20, 0);
    
    lv_obj_t *close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_icon, lv_color_white(), 0);
    lv_obj_center(close_icon);
    
    /* 关闭按钮事件 */
    lv_obj_add_event_cb(close_btn, close_detail_dialog_event_cb, LV_EVENT_CLICKED, dialog);
}

/**
 * @brief 从详情页添加到购物车事件回调
 * @param e 事件对象
 */
static void add_to_cart_from_detail_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        menu_item_t *item = (menu_item_t *)lv_event_get_user_data(e);
        add_item_to_cart(item);
        
        /* 关闭详情对话框 */
        lv_obj_t *dialog = lv_obj_get_parent(lv_event_get_target(e));
        lv_obj_del(dialog);
    }
}

/**
 * @brief 关闭详情对话框事件回调
 * @param e 事件对象
 */
static void close_detail_dialog_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *dialog = (lv_obj_t *)lv_event_get_user_data(e);
        lv_obj_del(dialog);
    }
}

/**
 * @brief 添加菜品到购物车
 * @param item 菜品数据
 */
static void add_item_to_cart(const menu_item_t *item)
{
    /* 检查购物车是否已满 */
    if (g_cart_item_count >= MAX_CART_ITEMS) {
        show_error_prompt("购物车已满");
        return;
    }
    
    /* 检查菜品是否已在购物车中 */
    for (int i = 0; i < g_cart_item_count; i++) {
        if (g_cart_items[i].item_id == item->item_id) {
            /* 增加数量 */
            g_cart_items[i].quantity++;
            update_cart_display();
            show_success_prompt("已添加到购物车");
            return;
        }
    }
    
    /* 添加新菜品到购物车 */
    cart_item_t *cart_item = &g_cart_items[g_cart_item_count];
    cart_item->item_id = item->item_id;
    strcpy(cart_item->name, item->name);
    cart_item->price = item->price;
    cart_item->quantity = 1;
    g_cart_item_count++;
    
    update_cart_display();
    show_success_prompt("已添加到购物车");
}

/**
 * @brief 更新购物车显示
 */
static void update_cart_display(void)
{
    /* 更新购物车数量标签 */
    lv_obj_t *count_label = lv_obj_get_child(g_cart_button, 1);
    char count_text[16];
    snprintf(count_text, sizeof(count_text), "%d", g_cart_item_count);
    lv_label_set_text(count_label, count_text);
    
    /* 更新购物车面板内容 */
    /* 这里应该更新购物车列表，暂时省略 */
}

/**
 * @brief 显示下单确认界面
 */
static void show_checkout_confirmation(void)
{
    /* 创建下单确认对话框 */
    lv_obj_t *dialog = lv_obj_create(g_main_screen);
    lv_obj_set_size(dialog, LV_PCT(70), LV_PCT(60));
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, lv_color_white(), 0);
    lv_obj_set_style_radius(dialog, 15, 0);
    lv_obj_set_style_shadow_width(dialog, 20, 0);
    
    /* 标题 */
    lv_obj_t *title = lv_label_create(dialog);
    lv_label_set_text(title, "确认下单");
    lv_obj_set_style_text_font(title, g_zh_font, 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    /* 订单详情 */
    lv_obj_t *details = lv_label_create(dialog);
    lv_label_set_text(details, "订单详情...");
    lv_obj_set_style_text_font(details, g_zh_font, 0);
    lv_obj_set_style_text_color(details, lv_color_hex(0x757575), 0);
    lv_obj_align(details, LV_ALIGN_TOP_MID, 0, 60);
    
    /* 确认下单按钮 */
    lv_obj_t *confirm_btn = lv_btn_create(dialog);
    lv_obj_set_size(confirm_btn, LV_PCT(60), 50);
    lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0xD32F2F), 0);
    lv_obj_set_style_radius(confirm_btn, 25, 0);
    
    lv_obj_t *confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "确认下单");
    lv_obj_set_style_text_font(confirm_label, g_zh_font, 0);
    lv_obj_set_style_text_color(confirm_label, lv_color_white(), 0);
    lv_obj_center(confirm_label);
    
    /* 确认按钮事件 */
    lv_obj_add_event_cb(confirm_btn, confirm_order_event_cb, LV_EVENT_CLICKED, NULL);
    
    /* 取消按钮 */
    lv_obj_t *cancel_btn = lv_btn_create(dialog);
    lv_obj_set_size(cancel_btn, LV_PCT(60), 50);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x9E9E9E), 0);
    lv_obj_set_style_radius(cancel_btn, 25, 0);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "取消");
    lv_obj_set_style_text_font(cancel_label, g_zh_font, 0);
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), 0);
    lv_obj_center(cancel_label);
    
    /* 取消按钮事件 */
    lv_obj_add_event_cb(cancel_btn, cancel_order_event_cb, LV_EVENT_CLICKED, dialog);
}

/**
 * @brief 确认下单事件回调
 * @param e 事件对象
 */
static void confirm_order_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* 创建订单 */
        create_order();
        
        /* 清空购物车 */
        clear_cart();
        
        /* 显示支付界面 */
        show_payment_interface();
    }
}

/**
 * @brief 取消下单事件回调
 * @param e 事件对象
 */
static void cancel_order_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *dialog = (lv_obj_t *)lv_event_get_user_data(e);
        lv_obj_del(dialog);
    }
}

/**
 * @brief 创建订单
 */
static void create_order(void)
{
    /* 这里应该创建订单 */
    /* 暂时只打印日志 */
    printf("创建订单\n");
}

/**
 * @brief 清空购物车
 */
static void clear_cart(void)
{
    g_cart_item_count = 0;
    update_cart_display();
}

/**
 * @brief 显示支付界面
 */
static void show_payment_interface(void)
{
    /* 创建支付界面 */
    lv_obj_t *payment_screen = lv_obj_create(g_main_screen);
    lv_obj_set_size(payment_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_center(payment_screen);
    lv_obj_set_style_bg_color(payment_screen, lv_color_hex(0xFFFFFF), 0);
    
    /* 支付标题 */
    lv_obj_t *title = lv_label_create(payment_screen);
    lv_label_set_text(title, "选择支付方式");
    lv_obj_set_style_text_font(title, g_zh_font, 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 50);
    
    /* 支付方式按钮 */
    const char *payment_methods[] = {"微信支付", "支付宝", "现金支付", "会员卡"};
    int method_count = sizeof(payment_methods) / sizeof(payment_methods[0]);
    
    for (int i = 0; i < method_count; i++) {
        lv_obj_t *btn = lv_btn_create(payment_screen);
        lv_obj_set_size(btn, 200, 60);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -100 + i * 80);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
        lv_obj_set_style_radius(btn, 30, 0);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, payment_methods[i]);
        lv_obj_set_style_text_font(label, g_zh_font, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);
        
        /* 支付方式按钮事件 */
        lv_obj_add_event_cb(btn, payment_method_btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/**
 * @brief 支付方式按钮事件回调
 * @param e 事件对象
 */
static void payment_method_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int method = (int)(intptr_t)lv_event_get_user_data(e);
        process_payment(method);
    }
}

/**
 * @brief 处理支付
 * @param payment_method 支付方式
 */
static void process_payment(int payment_method)
{
    /* 这里应该处理支付 */
    /* 暂时只打印日志 */
    printf("处理支付，方式: %d\n", payment_method);
    
    /* 显示支付成功提示 */
    show_success_prompt("支付成功");
}

/**
 * @brief 显示成功提示
 * @param message 提示信息
 */
static void show_success_prompt(const char *message)
{
    /* 创建成功提示 */
    lv_obj_t *prompt = lv_obj_create(g_main_screen);
    lv_obj_set_size(prompt, 200, 80);
    lv_obj_center(prompt);
    lv_obj_set_style_bg_color(prompt, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(prompt, 10, 0);
    
    lv_obj_t *label = lv_label_create(prompt);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_font(label, g_zh_font, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    
    /* 3秒后自动关闭 */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, prompt);
    lv_anim_set_time(&anim, 3000);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&anim, anim_ready_cb);
    lv_anim_start(&anim);
}

/**
 * @brief 显示错误提示
 * @param message 提示信息
 */
static void show_error_prompt(const char *message)
{
    /* 创建错误提示 */
    lv_obj_t *prompt = lv_obj_create(g_main_screen);
    lv_obj_set_size(prompt, 200, 80);
    lv_obj_center(prompt);
    lv_obj_set_style_bg_color(prompt, lv_color_hex(0xFF5722), 0);
    lv_obj_set_style_radius(prompt, 10, 0);
    
    lv_obj_t *label = lv_label_create(prompt);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_font(label, g_zh_font, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    
    /* 3秒后自动关闭 */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, prompt);
    lv_anim_set_time(&anim, 3000);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&anim, anim_ready_cb);
    lv_anim_start(&anim);
}

/**
 * @brief 动画完成回调
 * @param anim 动画对象
 */
static void anim_ready_cb(lv_anim_t *anim)
{
    lv_obj_t *obj = (lv_obj_t *)anim->var;
    lv_obj_del(obj);
}
