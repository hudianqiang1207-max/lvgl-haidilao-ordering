#include "../ui.h"
#include "menu_data.h"
#include "font_setup.h"
#include "net_client.h"
#include <stdio.h>

#define CLR_RED       lv_color_hex(0xC9151E)
#define CLR_BG        lv_color_hex(0xF5F5F5)
#define CLR_WHITE     lv_color_hex(0xFFFFFF)
#define CLR_TEXT_DARK  lv_color_hex(0x333333)
#define CLR_PRICE      lv_color_hex(0xE53935)

static void show_net_hint(const char *msg);

/* 提示框自动消失回调 */
static void hint_timer_cb(lv_timer_t *t)
{
    lv_obj_t *mask = (lv_obj_t *)t->user_data;
    if (mask) lv_obj_del(mask);
}

static void back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_screen_load(ui_OrderScreen);
}

static void pay_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int method = (int)(intptr_t)lv_event_get_user_data(e);

    /* 1. 把购物车提交给服务器，生成订单 */
    char order_cmd[256] = "ORDER ";
    int pos = 6;
    int n = cart_get_entry_count();
    for (int i = 0; i < n; i++) {
        const cart_entry_t *ce = cart_get_entry(i);
        if (!ce) continue;
        int left = (int)sizeof(order_cmd) - pos - 1;
        if (left < 16) break;
        pos += snprintf(order_cmd + pos, left, "%s%d:%d",
                        i == 0 ? "" : ",", ce->menu_id, ce->quantity);
    }
    if (net_send_line("%s", order_cmd) != 0) {
        printf("[checkout] 订单提交失败: 网络未连接\n");
        show_net_hint("网络连接失败，请稍后再试");
        return;
    }

    char line[128];
    if (net_recv_line(line, sizeof(line), 3000) != 0) {
        printf("[checkout] 订单提交超时\n");
        show_net_hint("服务器响应超时");
        return;
    }

    int order_id = 0, total = 0;
    if (sscanf(line, "ORDER_OK %d %d", &order_id, &total) != 2) {
        printf("[checkout] 服务器返回: %s\n", line);
        if (strncmp(line, "ORDER_FAIL", 10) == 0) {
            show_net_hint("部分菜品已售罄，请调整后重试");
        } else {
            show_net_hint("下单失败");
        }
        return;
    }
    printf("[checkout] 订单 %d 已创建，金额 %d 元\n", order_id, total);

    /* 2. 发起支付 */
    if (net_send_line("PAY %d %d", order_id, method) != 0) {
        show_net_hint("支付请求发送失败");
        return;
    }
    if (net_recv_line(line, sizeof(line), 3000) != 0) {
        show_net_hint("支付响应超时");
        return;
    }
    if (sscanf(line, "PAY_OK %d", &order_id) != 1) {
        printf("[checkout] 支付失败: %s\n", line);
        show_net_hint("支付失败，请重试");
        return;
    }

    /* 3. 支付成功，进入支付/出货页 */
    extern void ui_PaymentScreen_screen_init(int method, int order_id);
    extern lv_obj_t *ui_PaymentScreen;
    ui_PaymentScreen_screen_init(method, order_id);
    lv_screen_load(ui_PaymentScreen);
}

/* 简单网络错误提示（覆盖在屏幕中央，1.8 秒后消失） */
static void show_net_hint(const char *msg)
{
    lv_obj_t *mask = lv_obj_create(lv_screen_active());
    lv_obj_set_size(mask, 360, 72);
    lv_obj_center(mask);
    lv_obj_set_style_bg_color(mask, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(mask, LV_OPA_90, 0);
    lv_obj_set_style_radius(mask, 12, 0);
    lv_obj_remove_flag(mask, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(mask);
    lv_label_set_text(lbl, msg);
    lv_obj_set_style_text_color(lbl, CLR_WHITE, 0);
    lv_obj_center(lbl);

    lv_timer_t *t = lv_timer_create(hint_timer_cb, 1800, mask);
    lv_timer_set_repeat_count(t, 1);
}

void ui_CheckoutScreen_screen_init(void)
{
    if (ui_CheckoutScreen) lv_obj_del(ui_CheckoutScreen);
    ui_CheckoutScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_CheckoutScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_CheckoutScreen, CLR_BG, 0);
    lv_obj_set_style_pad_all(ui_CheckoutScreen, 0, 0);

    lv_font_t *zh = font_get_zh();
    if (zh) lv_obj_set_style_text_font(ui_CheckoutScreen, zh, 0);

    /* header */
    lv_obj_t *hdr = lv_obj_create(ui_CheckoutScreen);
    lv_obj_set_size(hdr, 800, 60);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hdr, CLR_RED, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    /* 返回按钮（右下角，开发板左上角触摸坏了）——回到点餐页 */
    lv_obj_t *back = lv_button_create(ui_CheckoutScreen);
    lv_obj_set_size(back, 88, 44);
    lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -14, -12);
    lv_obj_set_style_bg_color(back, lv_color_hex(0xA01018), 0);
    lv_obj_set_style_radius(back, 8, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "< 返回");
    lv_obj_center(bl);
    lv_obj_set_style_text_color(bl, CLR_WHITE, 0);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "确认订单");
    lv_obj_set_style_text_color(title, CLR_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* 订单卡片 */
    lv_obj_t *card = lv_obj_create(ui_CheckoutScreen);
    lv_obj_set_size(card, 480, 336);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, CLR_WHITE, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_opa(card, 16, 0);
    lv_obj_set_style_pad_all(card, 16, 0);

    lv_obj_t *card_ttl = lv_label_create(card);
    lv_label_set_text(card_ttl, "A18桌  4人用餐");
    lv_obj_set_style_text_color(card_ttl, CLR_TEXT_DARK, 0);
    lv_obj_align(card_ttl, LV_ALIGN_TOP_LEFT, 0, 0);

    int y = 40;
    int n = cart_get_entry_count();
    int total = 0;
    for (int i = 0; i < n; i++) {
        const cart_entry_t *ce = cart_get_entry(i);
        if (!ce) continue;
        const menu_item_t *mi = menu_find_by_id(ce->menu_id);
        if (!mi) continue;

        char buf[64];
        lv_snprintf(buf, sizeof(buf), "%s  x%d", mi->name, ce->quantity);
        lv_obj_t *nm = lv_label_create(card);
        lv_label_set_text(nm, buf);
        lv_obj_set_style_text_color(nm, CLR_TEXT_DARK, 0);
        lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 0, y);

        int subtotal = mi->price * ce->quantity;
        total += subtotal;
        char pbuf[16];
        lv_snprintf(pbuf, sizeof(pbuf), "%d元", subtotal);
        lv_obj_t *pr = lv_label_create(card);
        lv_label_set_text(pr, pbuf);
        lv_obj_set_style_text_color(pr, CLR_PRICE, 0);
        lv_obj_align(pr, LV_ALIGN_TOP_RIGHT, 0, y);

        y += 34;
    }

    /* 分割线 */
    lv_obj_t *sep = lv_obj_create(card);
    lv_obj_set_size(sep, 448, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_LEFT, 0, y + 8);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    /* 合计 */
    lv_obj_t *tot = lv_label_create(card);
    lv_label_set_text(tot, "合计");
    lv_obj_set_style_text_color(tot, lv_color_hex(0x666666), 0);
    lv_obj_align(tot, LV_ALIGN_TOP_LEFT, 0, y + 22);

    char tbuf[16];
    lv_snprintf(tbuf, sizeof(tbuf), "%d元", total);
    lv_obj_t *totv = lv_label_create(card);
    lv_label_set_text(totv, tbuf);
    lv_obj_set_style_text_color(totv, CLR_PRICE, 0);
    lv_obj_align(totv, LV_ALIGN_TOP_RIGHT, 0, y + 22);

    /* 支付方式按钮 */
    const char *methods[] = {"微信支付", "支付宝", "现金支付"};
    int ids[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(ui_CheckoutScreen);
        lv_obj_set_size(btn, 140, 48);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, -160 + i * 160, 424);
        lv_obj_set_style_bg_color(btn, CLR_RED, 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_shadow_width(btn, 4, 0);
        lv_obj_set_style_shadow_opa(btn, 20, 0);
        lv_obj_add_event_cb(btn, pay_cb, LV_EVENT_CLICKED, (void *)(intptr_t)ids[i]);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, methods[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, CLR_WHITE, 0);
    }
}
