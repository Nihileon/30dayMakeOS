/*
 * @Author: Nihil Eon
 * @Date: 2019-03-13 10:39:01
 * @Last Modified by: Nihil Eon
 * @Last Modified time: 2019-03-13 14:53:00
 */
#include "bootpack.h"
#include <stdio.h>

void init_pic(void)
{
    io_out8(PIC0_IMR, 0xff); //禁用中断
    io_out8(PIC1_IMR, 0xff); //禁用中断

    io_out8(PIC0_ICW1, 0x11);   // 边沿触发模式 (edge trigger mode)
    io_out8(PIC0_ICW2, 0x20);   // IRQ0-7由INT20-27接受
    io_out8(PIC0_ICW3, 1 << 2); // PIC1 由 IRQ2 接受
    io_out8(PIC0_ICW4, 0x01);   // 无缓冲区域

    io_out8(PIC1_ICW1, 0x11); // 边沿触发模式 (edge trigger mode)
    io_out8(PIC1_ICW2, 0x28); // IRQ8-15由INT28-2f接受
    io_out8(PIC1_ICW3, 2);    // PIC1 由 IRQ2 接受
    io_out8(PIC1_ICW4, 0x01); // 无缓冲区域

    io_out8(PIC0_IMR, 0xfb); // 11111011 pic1以外全部禁止
    io_out8(PIC1_IMR, 0xff); // 1111111  禁止所有中断
    return;
}

#define PORT_KEYDAT 0x0060

struct FIFO8 keyinfo;

void inthandler21(int *esp)
{
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61);
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyinfo,data);
    return;
}

void inthandler2c(int *esp)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF,
                  "INT2c (IRQ-12): PS/2 mouse");
    for (;;) {
        io_hlt();
    }
}

/* PIC0中断的不完整策略 */
/* 这个中断在Athlon64X2上通过芯片组提供的便利，只需执行一次 */
/* 这个中断只是接收，不执行任何操作 */
/* 为什么不处理？
        →  因为这个中断可能是电气噪声引发的、只是处理一些重要的情况。*/
void inthandler27(int *esp)
{
    io_out8(PIC0_OCW2, 0x67);
    return;
}