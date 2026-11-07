#ifndef __FONT_SETUP_H__
#define __FONT_SETUP_H__

#include "../lvgl/lvgl.h"

/* 根据编译目标选择字体路径 */
#ifdef __arm__
    #define ZH_FONT_PATH "/root/simkai.ttf"
#else
    #define ZH_FONT_PATH "/home/kongbai/海底捞平板/simkai.ttf"
#endif

/* 初始化中文字体并设置给所有控件 */
void font_setup(void);

/* 初始化拼音输入法 */
void pinyin_ime_setup(void);

/* 获取中文字体指针 */
lv_font_t* font_get_zh(void);

/* 获取描述小字体指针（次要信息用，形成文字层级） */
lv_font_t* font_get_desc(void);

/* 获取带中文fallback的默认字体 */
lv_font_t* font_get_default_with_fallback(void);

#endif /* __FONT_SETUP_H__ */
