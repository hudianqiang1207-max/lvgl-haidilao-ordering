#ifndef __DISPLAY_INIT_H__
#define __DISPLAY_INIT_H__

#include "../lvgl/lvgl.h"

/* 初始化显示设备（framebuffer或SDL） */
void display_init(void);

/* 初始化输入设备（触摸屏或鼠标） */
void input_init(void);

#endif /* __DISPLAY_INIT_H__ */
