#include "bootpack.h"

struct FIFO8 keyfifo;

#define PORT_KEYSTA 0x0064 //键盘设备号
#define KEYSTA_SEND_NOTREADY 0x02 //所读取数据的倒数第二位为0时有效
#define KEYCMD_WRITE_MODE 0x60    //模式设定的指令
#define KBC_MODE 0x47             // 鼠标模式的模式号

void inthandler21(int *esp)
{
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61);
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo,data);
    return;
}

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

