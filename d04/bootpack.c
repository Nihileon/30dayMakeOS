/*
 * @Author: Nihil Eon
 * @Date: 2019-03-13 10:37:45
 * @Last Modified by: Nihil Eon
 * @Last Modified time: 2019-03-13 14:50:45
 */
#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyinfo;

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx, my;
    int i;
    init_gdtidt();
    init_pic();
    io_sti(); // sti是cli的逆指令, 执行sti后, if(中断许可标志)变为1,
              // cpu可以接受外部中断
    fifo8_init(&keyinfo, 32, keybuf);
    io_out8(PIC0_IMR, 0xf9); //中断许可11111001 , 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    for (;;) {
        io_cli();
        if(fifo8_status(&keyinfo) == 0){
            io_stihlt();
        }else{
            i = fifo8_get(&keyinfo);
            io_sti();
            sprintf(s, "%02x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}
