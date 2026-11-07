/**
 * @file display_init.c
 * @brief SDL 显示和输入设备初始化
 */

#include <stdio.h>
#include <stdlib.h>
#include "display_init.h"

#if LV_USE_LINUX_FBDEV
/* 开发板触摸屏 evdev 驱动（仅 FBDEV 模式需要） */
#include "../lvgl/src/drivers/evdev/lv_evdev.h"
#endif

void display_init(void)
{
#if LV_USE_SDL
    /* SDL 环境检查 */
    if (getenv("DISPLAY") == NULL && getenv("WAYLAND_DISPLAY") == NULL) {
        printf("警告: 未检测到图形显示环境\n");
        printf("请执行以下操作之一:\n");
        printf("  1. 使用启动脚本: ./run_haidilao.sh\n");
        printf("  2. 在图形桌面环境中运行\n");
        printf("  3. WSL下运行 XLaunch 并设置 DISPLAY=:0\n");
        printf("  4. 安装 Xvfb: sudo apt install xvfb\n");
        printf("     然后: Xvfb :99 -screen 0 800x480x24 &\n");
        printf("     然后: export DISPLAY=:99\n");
        exit(1);
    }

    lv_display_t *disp = lv_sdl_window_create(800, 480);
    if (disp == NULL) {
        printf("错误: SDL窗口创建失败，请检查SDL2是否正确安装\n");
        printf("  sudo apt install libsdl2-dev\n");
        exit(1);
    }
    printf("SDL窗口创建完成 (800x480)\n");
#elif LV_USE_LINUX_FBDEV
    lv_display_t *disp = lv_linux_fbdev_create();
    if (disp == NULL) {
        printf("错误: framebuffer设备创建失败\n");
        exit(1);
    }
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    printf("framebuffer显示初始化完成\n");
#else
    #error "需要配置LV_USE_SDL=1或LV_USE_LINUX_FBDEV=1"
#endif
}

void input_init(void)
{
#if LV_USE_SDL
    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
    lv_sdl_mousewheel_create();
    printf("SDL输入设备初始化完成\n");
#elif LV_USE_LINUX_FBDEV
    const char *touch_devices[] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        NULL
    };
    
    lv_indev_t *touch = NULL;
    for (int i = 0; touch_devices[i] != NULL; i++) {
        touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_devices[i]);
        if (touch != NULL) {
            printf("触摸屏设备: %s\n", touch_devices[i]);
            break;
        }
    }
    
    if (touch != NULL) {
        lv_evdev_set_calibration(touch, 0, 0, 800, 480);
        printf("触摸屏初始化完成，校准参数: 0, 0, 800, 480\n");
    } else {
        printf("警告：触摸屏初始化失败\n");
    }
#endif
}
