#include "../ui.h"
#include "menu_data.h"
#include "font_setup.h"

/* ---------- 海底捞配色 ---------- */
#define CLR_RED         lv_color_hex(0xC9151E)
#define CLR_BG          lv_color_hex(0xF5F5F5)
#define CLR_WHITE       lv_color_hex(0xFFFFFF)
#define CLR_LIGHT_RED   lv_color_hex(0xFFF0F0)
#define CLR_BORDER      lv_color_hex(0xE0E0E0)
#define CLR_TEXT_DARK    lv_color_hex(0x333333)
#define CLR_TEXT_GRAY    lv_color_hex(0x888888)
#define CLR_TEXT_HINT    lv_color_hex(0xBBBBBB)
#define CLR_LINE         lv_color_hex(0xEEEEEE)
#define CLR_PRICE        lv_color_hex(0xE53935)
#define CLR_DIVIDER      lv_color_hex(0xF0F0F0)

/* ---------- 尺寸常量 ---------- */
#define HEADER_H      60
#define SIDEBAR_W     120
#define CART_W        220
#define CARD_W        210
#define CARD_H        196
#define CARD_GAP      14
#define SIDE_PAD      12
#define BTN_H         46

/* ---------- 持久对象 ---------- */
static lv_obj_t *g_product_area;
static lv_obj_t *g_sidebar;
static lv_obj_t *g_cart_content;
static lv_obj_t *g_cart_total_lbl;
static lv_obj_t *g_cart_count_lbl;
static lv_obj_t *g_cat_btns[CAT_COUNT];
static lv_obj_t *g_pot_lbl;   /* 头部已选锅底标签 */
static int g_current_cat = 0;

/* ---------- 前向声明 ---------- */
static void refresh_products(void);
static void refresh_cart(void);
static void sync_timer_cb(lv_timer_t *timer);
static void cat_cb(lv_event_t *e);
static void add_cb(lv_event_t *e);
static void minus_cb(lv_event_t *e);
static void plus_cb(lv_event_t *e);
static void checkout_cb(lv_event_t *e);
static void highlight_cat(void);
static void pot_card_cb(lv_event_t *e);

/* ================================================================
 *  构建: 顶部标题栏
 * ================================================================ */
static void build_header(lv_obj_t *parent)
{
    lv_font_t *zh = font_get_zh();
    lv_obj_t *h = lv_obj_create(parent);
    lv_obj_set_size(h, 800, HEADER_H);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(h, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(h, CLR_RED, 0);
    lv_obj_set_style_border_width(h, 0, 0);
    lv_obj_set_style_radius(h, 0, 0);
    lv_obj_set_style_pad_all(h, 0, 0);
    if (zh) lv_obj_set_style_text_font(h, zh, 0);

    /* 品牌 */
    lv_obj_t *brand = lv_label_create(h);
    lv_label_set_text(brand, "海底捞  智慧点餐");
    lv_obj_set_style_text_color(brand, CLR_WHITE, 0);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 16, 0);

    /* 已选锅底（居中显示） */
    g_pot_lbl = lv_label_create(h);
    lv_label_set_text(g_pot_lbl, "请选择锅底");
    lv_obj_set_style_text_color(g_pot_lbl, lv_color_hex(0xFFE0E0), 0);
    lv_obj_align(g_pot_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_font_t *pf = font_get_desc();
    if (pf) lv_obj_set_style_text_font(g_pot_lbl, pf, 0);

    /* 桌号 */
    ui_TableInfo = lv_label_create(h);
    lv_label_set_text(ui_TableInfo, "A18桌  4人");
    lv_obj_set_style_text_color(ui_TableInfo, CLR_WHITE, 0);
    lv_obj_align(ui_TableInfo, LV_ALIGN_RIGHT_MID, -16, 0);
}

/* ================================================================
 *  构建: 左侧分类栏
 * ================================================================ */
static void build_sidebar(lv_obj_t *parent)
{
    lv_font_t *zh = font_get_zh();
    g_sidebar = lv_obj_create(parent);
    lv_obj_set_size(g_sidebar, SIDEBAR_W, 480 - HEADER_H);
    lv_obj_align(g_sidebar, LV_ALIGN_TOP_LEFT, 0, HEADER_H);
    lv_obj_remove_flag(g_sidebar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_sidebar, CLR_WHITE, 0);
    lv_obj_set_style_border_width(g_sidebar, 0, 0);
    lv_obj_set_style_radius(g_sidebar, 0, 0);
    lv_obj_set_style_pad_all(g_sidebar, 8, 0);
    if (zh) lv_obj_set_style_text_font(g_sidebar, zh, 0);
    lv_obj_set_style_pad_gap(g_sidebar, 8, 0);
    lv_obj_set_flex_flow(g_sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_sidebar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < CAT_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(g_sidebar);
        lv_obj_set_size(btn, SIDEBAR_W - 16, BTN_H);
        lv_obj_set_style_bg_color(btn, CLR_WHITE, 0);
        lv_obj_set_style_bg_color(btn, CLR_LIGHT_RED, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, cat_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, menu_category_names[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x666666), 0);

        g_cat_btns[i] = btn;
    }
    highlight_cat();
}

static void highlight_cat(void)
{
    for (int i = 0; i < CAT_COUNT; i++) {
        lv_obj_t *btn = g_cat_btns[i];
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (i == g_current_cat) {
            lv_obj_set_style_bg_color(btn, CLR_LIGHT_RED, 0);
            lv_obj_set_style_text_color(lbl, CLR_RED, 0);
        } else {
            lv_obj_set_style_bg_color(btn, CLR_WHITE, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x666666), 0);
        }
    }
}

/* 锅底单选：点击选择/切换锅底，刷新卡片高亮与头部显示 */
static void pot_card_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int id = (int)(intptr_t)lv_event_get_user_data(e);

    pot_select(id);

    /* 更新头部显示 */
    if (g_pot_lbl) lv_label_set_text(g_pot_lbl, pot_get_selected_name());

    /* 刷新菜品区（重绘所有卡片，让选中态高亮生效） */
    refresh_products();

    /* 刷新右侧购物车（锅底已计入订单，显示在购物车并参与金额计算） */
    refresh_cart();
}

/* ================================================================
 *  构建: 中间菜品区
 * ================================================================ */
static void build_product_area(lv_obj_t *parent)
{
    int area_w = 800 - SIDEBAR_W - CART_W;

    lv_font_t *zh = font_get_zh();
    g_product_area = lv_obj_create(parent);
    lv_obj_set_size(g_product_area, area_w, 480 - HEADER_H);
    lv_obj_align(g_product_area, LV_ALIGN_TOP_LEFT, SIDEBAR_W, HEADER_H);
    lv_obj_set_scroll_dir(g_product_area, LV_DIR_VER);
    lv_obj_set_style_bg_color(g_product_area, CLR_BG, 0);
    lv_obj_set_style_border_width(g_product_area, 0, 0);
    lv_obj_set_style_radius(g_product_area, 0, 0);
    lv_obj_set_style_pad_all(g_product_area, SIDE_PAD, 0);
    lv_obj_set_style_pad_gap(g_product_area, CARD_GAP, 0);
    lv_obj_set_flex_flow(g_product_area, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g_product_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    if (zh) lv_obj_set_style_text_font(g_product_area, zh, 0);
}

static void refresh_products(void)
{
    lv_obj_clean(g_product_area);
    lv_font_t *zh = font_get_zh();

    int count = menu_get_count(g_current_cat);
    int is_pot = (g_current_cat == CAT_POT);
    for (int i = 0; i < count; i++) {
        const menu_item_t *item = menu_get_item(g_current_cat, i);
        if (!item) continue;

        /* ---- 卡片（锅底分类略小，一屏多显示几张） ---- */
        lv_obj_t *card = lv_obj_create(g_product_area);
        lv_obj_set_size(card, is_pot ? CARD_W : CARD_W, is_pot ? 178 : CARD_H);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, CLR_WHITE, 0);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, CLR_BORDER, 0);
        lv_obj_set_style_shadow_width(card, 4, 0);
        lv_obj_set_style_shadow_opa(card, 12, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        if (zh) lv_obj_set_style_text_font(card, zh, 0);

        /* 锅底卡片：已选则高亮边框 + 显示"已选"标记 */
        if (is_pot && item->id == pot_get_selected()) {
            lv_obj_set_style_border_width(card, 3, 0);
            lv_obj_set_style_border_color(card, CLR_RED, 0);
            lv_obj_set_style_shadow_width(card, 10, 0);
            lv_obj_set_style_shadow_opa(card, 30, 0);
        }

        /* 卡片内容: 横向 flex，图片 + 右侧文字列（锅底与普通菜一致，避免纵向布局溢出） */
        lv_obj_t *body = lv_obj_create(card);
        lv_obj_set_size(body, LV_PCT(100), LV_PCT(100));
        lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(body, CLR_WHITE, 0);
        lv_obj_set_style_border_width(body, 0, 0);
        lv_obj_set_style_radius(body, 0, 0);
        lv_obj_set_style_pad_all(body, 0, 0);
        lv_obj_set_style_pad_gap(body, 10, 0);
        lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

        /* 菜品图片（圆角；锅底略小给文字留空间） */
        lv_obj_t *img = lv_image_create(body);
        lv_image_set_src(img, item->img_src);
        lv_obj_set_size(img, is_pot ? 64 : 84, is_pot ? 64 : 84);
        lv_obj_set_style_radius(img, 8, 0);
        lv_obj_set_style_clip_corner(img, true, 0);

        /* 右侧文字列（flex column，自动把价格压到底部） */
        lv_obj_t *col = lv_obj_create(body);
        lv_obj_set_flex_grow(col, 1);
        lv_obj_set_height(col, LV_PCT(100));
        lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(col, CLR_WHITE, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_radius(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_style_pad_gap(col, 4, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        if (zh) lv_obj_set_style_text_font(col, zh, 0);

        /* 菜名（最多两行，行距加大） */
        lv_obj_t *nm = lv_label_create(col);
        lv_label_set_text(nm, item->name);
        lv_obj_set_width(nm, LV_PCT(100));
        lv_label_set_long_mode(nm, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(nm, CLR_TEXT_DARK, 0);
        lv_obj_set_style_text_line_space(nm, 4, 0);

        /* 描述（小一号字体，次要信息） */
        lv_obj_t *desc = lv_label_create(col);
        lv_label_set_text(desc, item->desc);
        lv_obj_set_width(desc, LV_PCT(100));
        lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(desc, CLR_TEXT_GRAY, 0);
        lv_obj_set_style_text_line_space(desc, 3, 0);
        lv_obj_set_flex_grow(desc, 1);
        lv_font_t *df = font_get_desc();
        if (df) lv_obj_set_style_text_font(desc, df, 0);

        /* 底部行: 价格 + 按钮 */
        lv_obj_t *bot = lv_obj_create(col);
        lv_obj_set_width(bot, LV_PCT(100));
        lv_obj_set_height(bot, is_pot ? 26 : 30);
        lv_obj_remove_flag(bot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(bot, CLR_WHITE, 0);
        lv_obj_set_style_border_width(bot, 0, 0);
        lv_obj_set_style_radius(bot, 0, 0);
        lv_obj_set_style_pad_all(bot, 0, 0);
        lv_obj_set_flex_flow(bot, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(bot, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        char pbuf[16];
        lv_snprintf(pbuf, sizeof(pbuf), "%d元", item->price);
        lv_obj_t *pr = lv_label_create(bot);
        lv_label_set_text(pr, pbuf);
        lv_obj_set_style_text_color(pr, CLR_PRICE, 0);
        lv_font_t *pf = font_get_desc();
        if (pf) lv_obj_set_style_text_font(pr, pf, 0);   /* 价格用 18px 小字体，避免与按钮重叠 */

        if (is_pot) {
            /* 锅底：单选按钮（小号 46x30，价格用 18px 字体后 96px 宽可容纳） */
            lv_obj_t *sel = lv_button_create(bot);
            lv_obj_set_size(sel, 42, 26);
            lv_obj_set_style_bg_color(sel, CLR_RED, 0);
            lv_obj_set_style_radius(sel, 8, 0);
            lv_obj_set_style_shadow_width(sel, 0, 0);
            lv_obj_set_style_pad_all(sel, 0, 0);
            lv_obj_add_event_cb(sel, pot_card_cb, LV_EVENT_CLICKED, (void *)(intptr_t)item->id);
            lv_obj_t *plus = lv_label_create(sel);
            if (item->id == pot_get_selected())
                lv_label_set_text(plus, "已选");
            else
                lv_label_set_text(plus, "选择");
            lv_obj_center(plus);
            lv_obj_set_style_text_color(plus, CLR_WHITE, 0);
            lv_font_t *df2 = font_get_desc();
            if (df2) lv_obj_set_style_text_font(plus, df2, 0);
        } else if (menu_get_stock(item->id) > 0) {
            /* 有库存：显示 + 按钮 */
            lv_obj_t *add = lv_button_create(bot);
            lv_obj_set_size(add, 30, 30);
            lv_obj_set_style_bg_color(add, CLR_RED, 0);
            lv_obj_set_style_radius(add, 8, 0);
            lv_obj_set_style_shadow_width(add, 0, 0);
            lv_obj_set_style_pad_all(add, 0, 0);
            lv_obj_add_event_cb(add, add_cb, LV_EVENT_CLICKED, (void *)(intptr_t)item->id);
            lv_obj_t *plus = lv_label_create(add);
            lv_label_set_text(plus, "+");
            lv_obj_center(plus);
            lv_obj_set_style_text_color(plus, CLR_WHITE, 0);
        } else {
            /* 售罄：灰色标签替代 + 按钮 */
            lv_obj_t *sold = lv_label_create(bot);
            lv_label_set_text(sold, "售罄");
            lv_obj_set_style_text_color(sold, CLR_TEXT_HINT, 0);
            lv_font_t *df = font_get_desc();
            if (df) lv_obj_set_style_text_font(sold, df, 0);
        }
    }
}

/* ================================================================
 *  构建: 右侧购物车
 * ================================================================ */
static void build_cart(lv_obj_t *parent)
{
    lv_font_t *zh = font_get_zh();
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, CART_W, 480 - HEADER_H);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, HEADER_H);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, CLR_WHITE, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    if (zh) lv_obj_set_style_text_font(panel, zh, 0);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 顶部标题行 */
    lv_obj_t *hdr = lv_obj_create(panel);
    lv_obj_set_size(hdr, CART_W, 48);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hdr, CLR_WHITE, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    lv_obj_t *ttl = lv_label_create(hdr);
    lv_label_set_text(ttl, "本桌已点");
    lv_obj_set_style_text_color(ttl, CLR_TEXT_DARK, 0);
    lv_obj_align(ttl, LV_ALIGN_LEFT_MID, 12, 0);

    /* 数量 badge */
    lv_obj_t *badge = lv_obj_create(hdr);
    lv_obj_set_size(badge, 38, 24);
    lv_obj_align(badge, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_bg_color(badge, CLR_LIGHT_RED, 0);
    lv_obj_set_style_radius(badge, 10, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    g_cart_count_lbl = lv_label_create(badge);
    lv_label_set_text(g_cart_count_lbl, "0");
    lv_obj_center(g_cart_count_lbl);
    lv_obj_set_style_text_color(g_cart_count_lbl, CLR_RED, 0);

    /* 分割线 */
    lv_obj_t *sep1 = lv_obj_create(panel);
    lv_obj_set_size(sep1, CART_W - 20, 1);
    lv_obj_set_style_bg_color(sep1, CLR_LINE, 0);
    lv_obj_set_style_border_width(sep1, 0, 0);

    /* 可滚动购物车列表 */
    g_cart_content = lv_obj_create(panel);
    lv_obj_set_size(g_cart_content, CART_W, 250);
    lv_obj_set_flex_grow(g_cart_content, 1);
    lv_obj_set_scroll_dir(g_cart_content, LV_DIR_VER);
    lv_obj_set_style_bg_color(g_cart_content, CLR_WHITE, 0);
    lv_obj_set_style_border_width(g_cart_content, 0, 0);
    lv_obj_set_style_radius(g_cart_content, 0, 0);
    lv_obj_set_style_pad_all(g_cart_content, 10, 0);
    lv_obj_set_style_pad_gap(g_cart_content, 10, 0);
    lv_obj_set_flex_flow(g_cart_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_cart_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* 分割线 */
    lv_obj_t *sep2 = lv_obj_create(panel);
    lv_obj_set_size(sep2, CART_W - 20, 1);
    lv_obj_set_style_bg_color(sep2, CLR_LINE, 0);
    lv_obj_set_style_border_width(sep2, 0, 0);

    /* 合计行 */
    lv_obj_t *total_row = lv_obj_create(panel);
    lv_obj_set_size(total_row, CART_W, 48);
    lv_obj_remove_flag(total_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(total_row, CLR_WHITE, 0);
    lv_obj_set_style_border_width(total_row, 0, 0);
    lv_obj_set_style_pad_all(total_row, 0, 0);

    lv_obj_t *tot_lbl = lv_label_create(total_row);
    lv_label_set_text(tot_lbl, "合计");
    lv_obj_set_style_text_color(tot_lbl, lv_color_hex(0x666666), 0);
    lv_obj_align(tot_lbl, LV_ALIGN_LEFT_MID, 12, 0);

    g_cart_total_lbl = lv_label_create(total_row);
    lv_label_set_text(g_cart_total_lbl, "0元");
    lv_obj_set_style_text_color(g_cart_total_lbl, CLR_PRICE, 0);
    lv_obj_align(g_cart_total_lbl, LV_ALIGN_RIGHT_MID, -12, 0);

    /* 结算按钮 */
    lv_obj_t *ck = lv_button_create(panel);
    lv_obj_set_size(ck, CART_W - 24, 52);
    lv_obj_set_style_bg_color(ck, CLR_RED, 0);
    lv_obj_set_style_radius(ck, 12, 0);
    lv_obj_set_style_shadow_width(ck, 6, 0);
    lv_obj_set_style_shadow_opa(ck, 30, 0);
    lv_obj_add_event_cb(ck, checkout_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ck_lbl = lv_label_create(ck);
    lv_label_set_text(ck_lbl, "去结算");
    lv_obj_center(ck_lbl);
    lv_obj_set_style_text_color(ck_lbl, CLR_WHITE, 0);

    /* 提示 */
    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "预计用餐90分钟");
    lv_obj_set_style_text_color(hint, CLR_TEXT_HINT, 0);
    { lv_font_t *df = font_get_desc(); if (df) lv_obj_set_style_text_font(hint, df, 0); }

    refresh_cart();
}

static void refresh_cart(void)
{
    /* 数量 */
    char cbuf[8];
    lv_snprintf(cbuf, sizeof(cbuf), "%d", cart_get_count());
    lv_label_set_text(g_cart_count_lbl, cbuf);

    /* 合计 */
    char tbuf[16];
    lv_snprintf(tbuf, sizeof(tbuf), "%d元", cart_get_total());
    lv_label_set_text(g_cart_total_lbl, tbuf);

    /* 列表 */
    lv_obj_clean(g_cart_content);

    int n = cart_get_entry_count();
    if (n == 0) {
        lv_obj_t *empty = lv_label_create(g_cart_content);
        lv_label_set_text(empty, "还没点菜~");
        lv_obj_set_style_text_color(empty, CLR_TEXT_HINT, 0);
        return;
    }

    for (int i = 0; i < n; i++) {
        const cart_entry_t *ce = cart_get_entry(i);
        if (!ce) continue;
        const menu_item_t *mi = menu_find_by_id(ce->menu_id);
        if (!mi) continue;

        /* 行容器 */
        lv_obj_t *row = lv_obj_create(g_cart_content);
        lv_obj_set_size(row, CART_W - 12, 46);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(row, CLR_DIVIDER, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);

        /* 菜名 */
        lv_obj_t *nm = lv_label_create(row);
        lv_label_set_text(nm, mi->name);
        lv_obj_set_width(nm, 86);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(nm, CLR_TEXT_DARK, 0);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 8, 0);

        /* - 按钮 */
        lv_obj_t *minus = lv_button_create(row);
        lv_obj_set_size(minus, 28, 28);
        lv_obj_align(minus, LV_ALIGN_LEFT_MID, 98, 0);
        lv_obj_set_style_bg_color(minus, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_radius(minus, 6, 0);
        lv_obj_set_style_shadow_width(minus, 0, 0);
        lv_obj_add_event_cb(minus, minus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)mi->id);
        lv_obj_t *ml = lv_label_create(minus);
        lv_label_set_text(ml, "-");
        lv_obj_center(ml);

        /* 数量 */
        char qbuf[4];
        lv_snprintf(qbuf, sizeof(qbuf), "%d", ce->quantity);
        lv_obj_t *qty = lv_label_create(row);
        lv_label_set_text(qty, qbuf);
        lv_obj_set_style_text_color(qty, CLR_TEXT_DARK, 0);
        lv_obj_align(qty, LV_ALIGN_LEFT_MID, 130, 0);

        /* + 按钮 */
        lv_obj_t *plus = lv_button_create(row);
        lv_obj_set_size(plus, 28, 28);
        lv_obj_align(plus, LV_ALIGN_LEFT_MID, 152, 0);
        lv_obj_set_style_bg_color(plus, CLR_RED, 0);
        lv_obj_set_style_radius(plus, 6, 0);
        lv_obj_set_style_shadow_width(plus, 0, 0);
        lv_obj_add_event_cb(plus, plus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)mi->id);
        lv_obj_t *pl = lv_label_create(plus);
        lv_label_set_text(pl, "+");
        lv_obj_center(pl);
        lv_obj_set_style_text_color(pl, CLR_WHITE, 0);
    }
}

/* ================================================================
 *  屏幕初始化入口
 * ================================================================ */
void ui_OrderScreen_screen_init(void)
{
    menu_data_init();

    ui_OrderScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_OrderScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_OrderScreen, CLR_BG, 0);
    lv_obj_set_style_pad_all(ui_OrderScreen, 0, 0);

    /* 在创建子对象前设置中文字体 (LV_PART_MAIN=0) */
    lv_font_t *zh = font_get_zh();
    if (zh) {
        lv_obj_set_style_text_font(ui_OrderScreen, zh, 0);
    }

    build_header(ui_OrderScreen);
    build_sidebar(ui_OrderScreen);
    build_product_area(ui_OrderScreen);
    build_cart(ui_OrderScreen);
    refresh_products();
    /* 每 5 秒只检查版本；版本不变时不触碰任何 LVGL 对象。 */
    lv_timer_create(sync_timer_cb, 5000, NULL);
}

static void sync_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    int changed = menu_sync_if_changed();
    if (changed > 0) {
        /* 只在完整菜单接收成功后重绘当前分类和购物车。 */
        refresh_products();
        refresh_cart();
        if (g_pot_lbl) lv_label_set_text(g_pot_lbl, pot_get_selected_name());
        printf("[menu] 后台同步完成，当前分类已刷新\n");
    }
}

/* ================================================================
 *  事件回调
 * ================================================================ */
static void cat_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    g_current_cat = (int)(intptr_t)lv_event_get_user_data(e);
    highlight_cat();
    refresh_products();
}

static void add_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    if (menu_get_stock(id) <= 0) return;   /* 售罄不可加购 */
    cart_add(id);
    refresh_cart();
}

static void minus_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    cart_remove((int)(intptr_t)lv_event_get_user_data(e));
    refresh_cart();
}

static void plus_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    cart_add((int)(intptr_t)lv_event_get_user_data(e));
    refresh_cart();
}

static void checkout_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (cart_get_count() == 0) return;
    extern void ui_CheckoutScreen_screen_init(void);
    extern lv_obj_t *ui_CheckoutScreen;
    ui_CheckoutScreen_screen_init();
    lv_screen_load(ui_CheckoutScreen);
}

