#include <stdio.h>
#include "bootpack.h"

#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15


void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base,
                  int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256];
    int mx, my;

    init_gdtidt();
    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    for (;;) {
        io_hlt();
    }
}
