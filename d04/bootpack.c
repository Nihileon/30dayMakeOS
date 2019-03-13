/*
 * @Author: Nihil Eon
 * @Date: 2019-03-13 10:37:45
 * @Last Modified by: Nihil Eon
 * @Last Modified time: 2019-03-13 14:50:45
 */
#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my;
    int i;

    init_gdtidt();
    init_pic();
    io_sti(); // sti是cli的逆指令, 执行sti后, if(中断许可标志)变为1,
              // cpu可以接受外部中断
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); //中断许可11111001 , 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)

    init_keyboard();

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    enable_mouse();
    for (;;) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%02x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();
                sprintf(s, "%02X", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 47,
                         31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF,
                              s);
            }
        }
    }
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064 //键盘设备号
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02 //所读取数据的倒数第二位为0时有效
#define KEYCMD_WRITE_MODE 0x60 //模式设定的指令
#define KBC_MODE 0x47          // 鼠标模式的模式号

//等待键盘控制电路有效
void wait_KBC_sendready(void)
{
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

void init_keyboard(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUSE 0XD4
#define MOUSECMD_ENABLE 0XF4

void enable_mouse(void)
{
    wait_KBC_sendready();
    io_out8(
        PORT_KEYCMD,
        KEYCMD_SENDTO_MOUSE); //往键盘控制电路发送指令0xd4，下一个数据就会自动发送给鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return
    /* 顺利的话,键盘控制其会返送回ACK(0xfa)*/
    //鼠标收到激活指令以后，马上就给CPU发送答复信息0xfa
}