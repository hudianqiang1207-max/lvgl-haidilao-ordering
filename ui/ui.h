#ifndef _HAIDILAO_UI_H
#define _HAIDILAO_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../lvgl/lvgl.h"
#include "menu_data.h"

/* OrderScreen objects */
extern lv_obj_t * ui_OrderScreen;
extern lv_obj_t * ui_HeaderTitle;
extern lv_obj_t * ui_TableInfo;

/* CheckoutScreen */
extern lv_obj_t * ui_CheckoutScreen;
void ui_CheckoutScreen_screen_init(void);

/* PaymentScreen */
extern lv_obj_t * ui_PaymentScreen;
void ui_PaymentScreen_screen_init(int method, int order_id);

/* image assets */
LV_IMG_DECLARE(ui_img_beef);
LV_IMG_DECLARE(ui_img_shrimp);
LV_IMG_DECLARE(ui_img_vegetables);
LV_IMG_DECLARE(ui_img_noodle);

/* init functions */
void ui_OrderScreen_screen_init(void);
void ui_init(void);

#ifdef __cplusplus
}
#endif
#endif
