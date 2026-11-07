#include "../ui.h"
#include "menu_data.h"
#include "font_setup.h"
#include "net_client.h"
#include <stdio.h>
#include <time.h>

#define CLR_RED       lv_color_hex(0xC9151E)
#define CLR_GREEN     lv_color_hex(0x4CAF50)
#define CLR_BG        lv_color_hex(0xF5F5F5)
#define CLR_WHITE     lv_color_hex(0xFFFFFF)
#define CLR_TEXT_DARK  lv_color_hex(0x333333)
#define CLR_TEXT_GRAY  lv_color_hex(0x888888)

static int g_pay_method = 1;
static int g_order_number = 0;

static void dispense_cb(lv_timer_t *timer);
static void show_dispense_result(lv_obj_t *parent, int ok);

static void done_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    /* 清空购物车并重建点餐界面（保证界面与数据一致） */
    cart_clear();
    extern void ui_OrderScreen_screen_init(void);
    ui_OrderScreen_screen_init();
    lv_screen_load(ui_OrderScreen);
}

static void timer_cb(lv_timer_t *timer)
{
    lv_obj_t *spinner = (lv_obj_t *)timer->user_data;
    lv_obj_t *parent = spinner ? lv_obj_get_parent(spinner) : NULL;
    if (!parent) return;

    /* 清空支付卡片旧内容（支付方式/金额/spinner/“正在处理支付”），避免与成功页元素叠加拥挤 */
    lv_obj_clean(parent);

    /* 成功图标：用 LVGL 内置字体渲染 ✓（FreeType 中文字体无此符号，缺字会显示成方框） */
    lv_obj_t *check = lv_label_create(parent);
    lv_label_set_text(check, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(check, &lv_font_montserrat_24, 0);
    lv_obj_align(check, LV_ALIGN_CENTER, 0, -72);
    lv_obj_set_style_text_color(check, CLR_GREEN, 0);

    /* 成功文字 */
    lv_obj_t *msg = lv_label_create(parent);
    lv_label_set_text(msg, "支付成功");
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -24);
    lv_obj_set_style_text_color(msg, CLR_GREEN, 0);

    /* 订单号 */
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "订单号: %d", g_order_number);
    lv_obj_t *order = lv_label_create(parent);
    lv_label_set_text(order, buf);
    lv_obj_set_style_text_color(order, CLR_TEXT_GRAY, 0);
    lv_obj_align(order, LV_ALIGN_CENTER, 0, 24);

    /* 提示：正在出货 */
    lv_obj_t *tip = lv_label_create(parent);
    lv_label_set_text(tip, "正在出货，请稍候...");
    lv_obj_set_style_text_color(tip, CLR_TEXT_GRAY, 0);
    lv_obj_align(tip, LV_ALIGN_CENTER, 0, 64);

    /* 0.8 秒后请求服务器出货 */
    lv_timer_t *dt = lv_timer_create(dispense_cb, 800, parent);
    lv_timer_set_repeat_count(dt, 1);
}

/* 出货回调：向服务器请求出货，按结果显示取餐/故障提示 */
static void dispense_cb(lv_timer_t *timer)
{
    lv_obj_t *parent = (lv_obj_t *)timer->user_data;
    if (!parent) return;

    if (net_send_line("DISPENSE %d", g_order_number) != 0) {
        printf("[payment] 出货请求发送失败\n");
        show_dispense_result(parent, 0);
        return;
    }
    char line[64];
    if (net_recv_line(line, sizeof(line), 3000) != 0) {
        printf("[payment] 出货响应超时\n");
        show_dispense_result(parent, 0);
        return;
    }
    printf("[payment] 服务器返回: %s\n", line);
    if (strncmp(line, "DISPENSE_OK", 11) == 0) {
        show_dispense_result(parent, 1);
    } else {
        show_dispense_result(parent, 0);
    }
}

/* 出货结果显示：ok=1 成功取餐，ok=0 故障提示 */
static void show_dispense_result(lv_obj_t *parent, int ok)
{
    /* 提示文字更新 */
    lv_obj_t *tip = lv_obj_get_child(parent, lv_obj_get_child_count(parent) - 1);
    if (tip && lv_obj_check_type(tip, &lv_label_class)) {
        if (ok) {
            lv_label_set_text(tip, "出货完成，请取餐！");
            lv_obj_set_style_text_color(tip, CLR_GREEN, 0);
        } else {
            lv_label_set_text(tip, "出货故障，请联系店员");
            lv_obj_set_style_text_color(tip, lv_color_hex(0xE53935), 0);
        }
    }

    /* 返回按钮 */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 160, 48);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 116);
    lv_obj_set_style_bg_color(btn, CLR_RED, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 4, 0);
    lv_obj_add_event_cb(btn, done_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "返回点餐");
    lv_obj_center(bl);
    lv_obj_set_style_text_color(bl, CLR_WHITE, 0);
}

void ui_PaymentScreen_screen_init(int method, int order_id)
{
    g_pay_method = method;
    g_order_number = order_id;

    if (ui_PaymentScreen) lv_obj_del(ui_PaymentScreen);
    ui_PaymentScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_PaymentScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PaymentScreen, CLR_BG, 0);
    lv_obj_set_style_pad_all(ui_PaymentScreen, 0, 0);

    lv_font_t *zh = font_get_zh();
    if (zh) lv_obj_set_style_text_font(ui_PaymentScreen, zh, 0);

    /* header */
    lv_obj_t *hdr = lv_obj_create(ui_PaymentScreen);
    lv_obj_set_size(hdr, 800, 60);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hdr, CLR_RED, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    /* 返回按钮（右下角，开发板左上角触摸坏了）——回到点餐页，清空购物车 */
    lv_obj_t *back = lv_button_create(ui_PaymentScreen);
    lv_obj_set_size(back, 96, 48);
    lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
    lv_obj_set_style_bg_color(back, lv_color_hex(0xA01018), 0);
    lv_obj_set_style_radius(back, 10, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_set_style_border_width(back, 1, 0);
    lv_obj_set_style_border_color(back, lv_color_hex(0xFFE0E0), 0);
    lv_obj_add_event_cb(back, done_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "返回");
    lv_obj_center(bl);
    lv_obj_set_style_text_color(bl, CLR_WHITE, 0);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "支付中...");
    lv_obj_set_style_text_color(title, CLR_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* 支付卡片 */
    lv_obj_t *card = lv_obj_create(ui_PaymentScreen);
    lv_obj_set_size(card, 360, 360);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, CLR_WHITE, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_opa(card, 16, 0);

    /* 支付方式 */
    const char *mname = "微信支付";
    if (g_pay_method == 2) mname = "支付宝";
    else if (g_pay_method == 3) mname = "现金支付";

    lv_obj_t *ml = lv_label_create(card);
    lv_label_set_text(ml, mname);
    lv_obj_set_style_text_color(ml, CLR_TEXT_DARK, 0);
    lv_obj_align(ml, LV_ALIGN_CENTER, 0, -92);

    /* 金额 */
    char abuf[16];
    lv_snprintf(abuf, sizeof(abuf), "%d元", cart_get_total());
    lv_obj_t *amt = lv_label_create(card);
    lv_label_set_text(amt, abuf);
    lv_obj_set_style_text_color(amt, CLR_RED, 0);
    lv_obj_align(amt, LV_ALIGN_CENTER, 0, -52);

    /* 加载动画 */
    lv_obj_t *spinner = lv_spinner_create(card);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 24);
    lv_spinner_set_anim_params(spinner, 1000, 200);

    /* 处理中 */
    lv_obj_t *proc = lv_label_create(card);
    lv_label_set_text(proc, "正在处理支付...");
    lv_obj_set_style_text_color(proc, CLR_TEXT_GRAY, 0);
    lv_obj_align(proc, LV_ALIGN_CENTER, 0, 82);

    /* 2秒后模拟支付成功 */
    lv_timer_t *t = lv_timer_create(timer_cb, 2000, spinner);
    lv_timer_set_repeat_count(t, 1);
}
