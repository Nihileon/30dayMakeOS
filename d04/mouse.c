#include "bootpack.h"


struct FIFO8 mousefifo;

void inthandler2c(int *esp)
{
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64); /* IRQ-12受付完了をPIC1に通知 */
    io_out8(PIC0_OCW2, 0x62); /* IRQ-02受付完了をPIC0に通知 */
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
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
        mdec->btn =
            mdec->buf[0] & 0x07; // buf[0] & 00000111 左右中键是buf[0]的后三位
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) { // 00010000
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) { // 00100000
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; // 鼠标移动方向与y轴相反
        return 1;
    default:
        return -1;
    }
}
