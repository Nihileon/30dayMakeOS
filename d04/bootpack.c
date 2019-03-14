/*
 * @Author: Nihil Eon
 * @Date: 2019-03-13 10:37:45
 * @Last Modified by: Nihil Eon
 * @Last Modified time: 2019-03-13 14:50:45
 */
#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
    unsigned char buf[3];
    unsigned char phase;
    int x, y, btn;//btn是左右中键
};

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my;
    int i;
    struct MOUSE_DEC mdec;

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
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    enable_mouse(&mdec);

    for (;;) {
        io_cli();

        if (fifo8_status(&keyfifo) != 0) {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%02x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        } else if (fifo8_status(&mousefifo) != 0) {
            i = fifo8_get(&mousefifo);
            io_sti();
            if (mouse_decode(&mdec, i) != 0) {
                sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                if ((mdec.btn & 0x01) != 0) { //左键
                    s[1] = 'L';
                }
                if ((mdec.btn & 0x02) != 0) { //右键
                    s[3] = 'R';
                }
                if ((mdec.btn & 0x04) != 0) { //中键
                    s[2] = 'C';
                }
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16,
                         32 + 15 * 8 - 1, 31); // 隐藏坐标, 008484 是背景色
                putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF,
                              s);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my,
                         mx + 15, my + 15); // 隐藏鼠标
                mx += mdec.x;
                my += mdec.y;
                if (mx < 0) {
                    mx = 0;
                }
                if (my < 0) {
                    my = 0;
                }
                if (mx > binfo->scrnx - 16) {
                    mx = binfo->scrnx - 16;
                }
                if (my > binfo->scrny - 16) {
                    my = binfo->scrny - 16;
                }
                sprintf(s, "(%3d, %3d)", mx, my);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79,
                         15); // 隐藏坐标
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); //显示坐标
                putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, //显示鼠标
                            16);
            }
        }else{
            io_stihlt();
        }
    }
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064 //键盘设备号
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02 //所读取数据的倒数第二位为0时有效
#define KEYCMD_WRITE_MODE 0x60    //模式设定的指令
#define KBC_MODE 0x47             // 鼠标模式的模式号

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

void enable_mouse(struct MOUSE_DEC *mdec)
{
    wait_KBC_sendready();
    io_out8(
        PORT_KEYCMD,
        KEYCMD_SENDTO_MOUSE); //往键盘控制电路发送指令0xd4，下一个数据就会自动发送给鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
    return;
    /* 顺利的话,键盘控制其会返送回ACK(0xfa)*/
    //鼠标收到激活指令以后，马上就给CPU发送答复信息0xfa
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    switch (mdec->phase) {
    case 0:
        //受到激活指令
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    case 1:
        // dat & 11001000 == 00001000
        if ((dat & 0xc8) == 0x08) {
            /* 正しい1バイト目だった */
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    case 2:
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    case 3:
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07; // buf[0] & 00000111 左右中键是buf[0]的后三位
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) { //00010000
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) { //00100000
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; // 鼠标移动方向与y轴相反
        return 1;
    default:
        return -1;
    }
}