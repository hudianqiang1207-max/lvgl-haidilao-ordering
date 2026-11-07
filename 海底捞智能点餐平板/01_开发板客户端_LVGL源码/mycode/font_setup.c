/**
 * @file font_setup.c
 * @brief 使用 FreeType 加载完整中文字体，避免精简 C 字库缺字。
 */

#include <stdio.h>
#include "font_setup.h"

static lv_font_t *g_zh_font = NULL;
static lv_font_t *g_desc_font = NULL;
static lv_font_t *g_default_font = NULL;

lv_font_t *font_get_zh(void) { return g_zh_font; }
lv_font_t *font_get_desc(void) { return g_desc_font; }
lv_font_t *font_get_default_with_fallback(void) { return g_default_font; }

void font_setup(void)
{
    /* 完整 TTF 由 FreeType 按需取字，覆盖菜单、订单和提示文案。
     * 主字体 24px 保证中文清晰可读，描述字体 18px 形成层级。 */
    g_zh_font = lv_freetype_font_create(
        ZH_FONT_PATH,
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        24,
        LV_FREETYPE_FONT_STYLE_NORMAL);
    if (g_zh_font == NULL) {
        printf("错误: 中文字体加载失败: %s\n", ZH_FONT_PATH);
        return;
    }

    g_desc_font = lv_freetype_font_create(
        ZH_FONT_PATH,
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        18,
        LV_FREETYPE_FONT_STYLE_NORMAL);
    if (g_desc_font == NULL) {
        printf("警告: 描述字体(18px)加载失败，回退主字体\n");
        g_desc_font = g_zh_font;
    }

    g_default_font = g_zh_font;
    printf("中文字体加载成功: FreeType simkai.ttf 24px / 描述 18px\n");
}

void pinyin_ime_setup(void)
{
    printf("拼音输入法功能已跳过\n");
}
