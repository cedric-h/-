// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2024-2026

// This example shows how to convert two different backgrounds to 8-bit bitmap
// mode. Each of them has their own bitmap, but they share the same palette.
//
// Be careful with your original images, they combined palette must have at most
// 256 colors.

#include <nds.h>

typedef enum {
    fb_PaletteColor_Background,
    fb_PaletteColor_Grid,
    fb_PaletteColor_Foreground,
} fb_PaletteColor;

const unsigned short palette[256]
__attribute__((aligned(4)))
__attribute__((visibility("hidden"))) = {
    [fb_PaletteColor_Background] = RGB5( 0,  0,  0),
    [fb_PaletteColor_Grid      ] = RGB5( 8,  8,  8),
    [fb_PaletteColor_Foreground] = RGB5(31, 31, 31),
};

static void fb_rect(uint16_t *fb, int x, int y) {
    x -= 1;
    y -= 1;
    for (int j = y; j < 2 + y; j++) {
        for (int i = x; i < 2 + x; i += 2) {
            /* sets two pixels at once */
            fb[256 * j / 2 + i / 2] =
                (fb_PaletteColor_Foreground << 0) |
                (fb_PaletteColor_Foreground << 8);
        }
    }
}

static void fb_set_px(uint16_t *fb, int x, int y, fb_PaletteColor color) {
    fb[256*y/2 + x/2] |= (color << 8*(x%2));
}

typedef struct { int min_x, max_x, min_y, max_y; } fb_Rect;
static void fb_rect_outline(
    uint16_t *fb,
    fb_Rect rect,
    fb_PaletteColor color
) {
    for (int x = rect.min_x; x < rect.max_x; x++)
        fb_set_px(fb, x, rect.min_y, color);

    for (int x = rect.min_x; x < rect.max_x; x++)
        fb_set_px(fb, x, rect.max_y, color);

    for (int y = rect.min_y; y <= rect.max_y; y++)
        fb_set_px(fb, rect.min_x, y, color);

    for (int y = rect.min_y; y <= rect.max_y; y++)
        fb_set_px(fb, rect.max_x, y, color);
}

int main(int argc, char *argv[]) {
    // Mode 5 lets you use layers 2 and 3 as a bitmap
    videoSetMode(MODE_5_2D), videoSetModeSub(MODE_5_2D);

    vramSetPrimaryBanks(
        VRAM_A_MAIN_BG, VRAM_B_LCD,
        VRAM_C_SUB_BG , VRAM_D_LCD
    );

    // Setup one background on each engine

    int bg = bgInitHidden(2, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    int bgsub = bgInitHiddenSub(2, BgType_Bmp8, BgSize_B8_256x256, 0, 0);

    memset(bgGetGfxPtr(bg), fb_PaletteColor_Background, 256*256);
    memset(bgGetGfxPtr(bgsub), fb_PaletteColor_Background, 256*256);

    memcpy(BG_PALETTE, palette, 512);
    memcpy(BG_PALETTE_SUB, palette, 512);

    bgShow(bg), bgShow(bgsub);

    int x = 40, y = 40;

    while (1) {
        swiWaitForVBlank();

        scanKeys();
        touchPosition touch_pos = { 0 };
        if (keysHeld() & KEY_TOUCH) {
            touchRead(&touch_pos);
            x = touch_pos.px;
            y = touch_pos.py;
        }

        fb_rect_outline(
            bgGetGfxPtr(bgsub),
            (fb_Rect) { x, x + 10, y, y + 10 },
            fb_PaletteColor_Foreground
        );
        fb_rect_outline(
            bgGetGfxPtr(bg),
            (fb_Rect) { x, x + 10, y, y + 10 },
            fb_PaletteColor_Foreground
        );
    }

    return 0;
}

