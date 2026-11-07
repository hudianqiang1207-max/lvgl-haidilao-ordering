#ifndef _MENU_DATA_H
#define _MENU_DATA_H

#include "lvgl/lvgl.h"   /* LV_IMG_DECLARE / lv_image_dsc_t */

#define MENU_MAX_ITEMS    80
#define MENU_MAX_NAME     32
#define MENU_MAX_DESC     48
#define CART_MAX_ITEMS    20

typedef struct {
    int id;
    char name[MENU_MAX_NAME];
    char desc[MENU_MAX_DESC];
    int price;
    int category;
    const void *img_src;
    int stock;              /* 库存数量，0 表示售罄 */
} menu_item_t;

typedef struct {
    int menu_id;
    int quantity;
} cart_entry_t;

/* category ids */
enum {
    CAT_POT = 0,        /* 锅底（新增，默认展示） */
    CAT_RECOMMEND,      /* 招牌精选 */
    CAT_MEAT,           /* 肉类 */
    CAT_SEAFOOD,        /* 海鲜 */
    CAT_VEG,            /* 蔬菜 */
    CAT_STAPLE,         /* 主食小吃 */
    CAT_DRINK,          /* 饮品 */
    CAT_COUNT
};

extern const char *menu_category_names[CAT_COUNT];

void menu_data_init(void);
int  menu_load_from_server(void);

/* 后台同步：成功下载完整菜单后返回 0；不触碰 LVGL 对象 */
int  menu_sync_if_changed(void);
int  menu_get_count(int category);
const menu_item_t *menu_get_item(int category, int index);
const menu_item_t *menu_find_by_id(int id);
int  menu_get_stock(int id);
void menu_set_stock(int id, int stock);

/* 锅底选择（单选） */
void pot_select(int id);
int  pot_get_selected(void);
const char *pot_get_selected_name(void);

/* cart operations */
void cart_clear(void);
void cart_add(int menu_id);
void cart_remove(int menu_id);
int  cart_get_count(void);
int  cart_get_total(void);
int  cart_get_entry_count(void);
const cart_entry_t *cart_get_entry(int index);

#endif

/* 真实菜品图片资源（来自海底捞官网 OSS） */
LV_IMG_DECLARE(ui_img_real_368);
LV_IMG_DECLARE(ui_img_real_3307);
LV_IMG_DECLARE(ui_img_real_3493);
LV_IMG_DECLARE(ui_img_real_3494);
LV_IMG_DECLARE(ui_img_real_3495);
LV_IMG_DECLARE(ui_img_real_7634);
LV_IMG_DECLARE(ui_img_real_7676);
LV_IMG_DECLARE(ui_img_real_428);
LV_IMG_DECLARE(ui_img_real_3510);
LV_IMG_DECLARE(ui_img_real_8267);
LV_IMG_DECLARE(ui_img_real_8268);
LV_IMG_DECLARE(ui_img_real_8269);
LV_IMG_DECLARE(ui_img_real_8270);
LV_IMG_DECLARE(ui_img_real_8271);
LV_IMG_DECLARE(ui_img_real_8272);
LV_IMG_DECLARE(ui_img_real_8273);
LV_IMG_DECLARE(ui_img_real_8679);
LV_IMG_DECLARE(ui_img_real_8306);
LV_IMG_DECLARE(ui_img_real_8307);
LV_IMG_DECLARE(ui_img_real_8308);
LV_IMG_DECLARE(ui_img_real_8309);
LV_IMG_DECLARE(ui_img_real_8310);
LV_IMG_DECLARE(ui_img_real_8311);
LV_IMG_DECLARE(ui_img_real_8312);
LV_IMG_DECLARE(ui_img_real_8313);
LV_IMG_DECLARE(ui_img_real_8314);
LV_IMG_DECLARE(ui_img_real_8315);
LV_IMG_DECLARE(ui_img_real_8316);
LV_IMG_DECLARE(ui_img_real_8317);
LV_IMG_DECLARE(ui_img_real_8318);
LV_IMG_DECLARE(ui_img_real_8319);
LV_IMG_DECLARE(ui_img_real_8320);
LV_IMG_DECLARE(ui_img_real_8499);
LV_IMG_DECLARE(ui_img_real_8502);
LV_IMG_DECLARE(ui_img_real_8503);
LV_IMG_DECLARE(ui_img_real_8504);
LV_IMG_DECLARE(ui_img_real_8498);
LV_IMG_DECLARE(ui_img_real_8501);
LV_IMG_DECLARE(ui_img_real_8674);
LV_IMG_DECLARE(ui_img_real_8675);
LV_IMG_DECLARE(ui_img_real_8676);
LV_IMG_DECLARE(ui_img_real_8677);
LV_IMG_DECLARE(ui_img_real_8678);
LV_IMG_DECLARE(ui_img_real_130);
LV_IMG_DECLARE(ui_img_real_3604);
LV_IMG_DECLARE(ui_img_real_3608);
LV_IMG_DECLARE(ui_img_real_8321);
LV_IMG_DECLARE(ui_img_real_8322);
LV_IMG_DECLARE(ui_img_real_8525);
LV_IMG_DECLARE(ui_img_real_8526);
LV_IMG_DECLARE(ui_img_real_456);
LV_IMG_DECLARE(ui_img_real_3325);
LV_IMG_DECLARE(ui_img_real_3594);
LV_IMG_DECLARE(ui_img_real_3595);
LV_IMG_DECLARE(ui_img_real_3596);
LV_IMG_DECLARE(ui_img_real_3597);
LV_IMG_DECLARE(ui_img_real_3598);
LV_IMG_DECLARE(ui_img_real_3599);
LV_IMG_DECLARE(ui_img_real_40);
LV_IMG_DECLARE(ui_img_real_3618);
LV_IMG_DECLARE(ui_img_real_3762);
LV_IMG_DECLARE(ui_img_real_21);   /* 酸梅汤（用户实物图） */
