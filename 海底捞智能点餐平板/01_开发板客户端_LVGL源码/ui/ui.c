#include "ui.h"

lv_obj_t * ui_OrderScreen;
lv_obj_t * ui_HeaderTitle;
lv_obj_t * ui_TableInfo;
lv_obj_t * ui_CheckoutScreen;
lv_obj_t * ui_PaymentScreen;

void ui_init(void)
{
    ui_CheckoutScreen = NULL;
    ui_PaymentScreen = NULL;

    ui_OrderScreen_screen_init();
    lv_screen_load(ui_OrderScreen);
}
