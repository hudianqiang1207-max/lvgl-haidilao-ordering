/**
 * @file main_haidilao.c
 * @brief 海底捞点餐系统 - 主程序
 */

#include <stdio.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "./ui/ui.h"
#include "./mycode/display_init.h"
#include "./mycode/font_setup.h"

int main(void)
{
    printf("=== 海底捞点餐系统启动 ===\n");
    
    printf("[1/5] lv_init...\n");
    lv_init();

    printf("[2/5] display_init...\n");
    display_init();

    printf("[3/5] input_init...\n");
    input_init();

    printf("[4/5] font_setup...\n");
    font_setup();
    
    printf("[5/5] ui_init...\n");
    ui_init();
    
    /* ui_init 后在当前活动屏上设置中文字体 */
    {
        lv_font_t *zh = font_get_zh();
        if (zh) {
            lv_obj_t *scr = lv_screen_active();
            lv_obj_set_style_text_font(scr, zh, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_report_style_change(NULL);
            printf("全局中文字体已应用 (FreeType simkai.ttf)\n");
        }
    }
    
    printf("=== 初始化完成，开始主循环 ===\n");

    while (1) {
        lv_timer_handler();
        usleep(5000);
    }
    return 0;
}


