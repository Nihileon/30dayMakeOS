; haribote-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; 加载bootpack
DSKCAC	EQU		0x00100000		; 磁盘缓存的位置
DSKCAC0	EQU		0x00008000		; 磁盘缓存的位置（实模式）

; BOOT_INFO相关
CYLS	EQU		0x0ff0			; 引导扇区设置
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 关于颜色的信息
SCRNX	EQU		0x0ff4			; 分辨率X
SCRNY	EQU		0x0ff6			; 分辨率Y
VRAM	EQU		0x0ff8			; 图像缓冲区的起始地址

		ORG		0xc200			;  这个的程序要被装载的内存地址

; 画面モードを設定

		MOV		AL,0x13			; VGA显卡，320x200x8bit
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 屏幕的模式（参考C语言的引用）
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 通过BIOS获取指示灯状态

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; 防止PIC接受所有中断
;	AT兼容机的规范、PIC初始化
;	然后之前在CLI不做任何事就挂起
;	PIC在同意后初始化

; io_out(PIC0_IMR, 0XFF);
; io_out(PIC1_IMR, 0XFF);
		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 不断执行OUT指令
		OUT		0xa1,AL

; io_cli();
		CLI						; 进一步中断CPU


; 设置A20GATE信号线为ON, 作用是使得内存1MB以上部分可用
; #define KEYCMD_WRITE_OUTPORT 0XD1
; #define KBC_OUTPORT_A20G_ENABLE 0XDF
; wait_KBC_sendready();
		CALL	waitkbdout
; io_out8(PORT_KEYCMD, KEYCMD_WRITE_OUTPORT);
		MOV		AL,0xd1
		OUT		0x64,AL
; wait_KBC_sendready();
		CALL	waitkbdout
; io_out8(PORT_KEYDAT, KBC_OUTPORT_A20G_ENABLE);
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
; wait_KBC_sendready();    这句话是为了等待指令执行完成
		CALL	waitkbdout

; 保护模式转换

[INSTRSET "i486p"]				; 为了能使用386之后的LGDT, EAX, CR0等关键字

		LGDT	[GDTR0]			; 设置临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设bit31为0（禁用分页）
		OR		EAX,0x00000001	; 设bit0为1 (切换到保护模式)
		MOV		CR0,EAX
		; 由于cpu使用管道进行流水处理,
		; 所以当我们切换为32位模式改变了指令的时候需要重新解释一遍
		JMP		pipelineflush
; 除CS以外的所有段寄存器值都变为了0x0008, 相当于gdt+1的段
; 而CS不变是因为防止发生混乱
pipelineflush:
		MOV		AX,1*8			;  写32bit的段
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack传递
; memcpy(bootpack, BOTPAK, 512*1024/4)
		MOV		ESI,bootpack	; 源
		MOV		EDI,BOTPAK		; 目标
		MOV		ECX,512*1024/4  ; 大小
		CALL	memcpy

; 传输磁盘数据

; 从引导区开始
; memcpy(0x7c00, DSKCAC, 512/4)
; DSKCAC是0x00100000，这句意思是从0x7c00复制512Byte到0x00100000
		MOV		ESI,0x7c00		; 源
		MOV		EDI,DSKCAC		; 目标
		MOV		ECX,512/4
		CALL	memcpy

; 剩余的全部
; 意思就是将始于0x00008200的磁盘内容，复制到0x00100200那里
;memcpy(DSKCAC0+512, DSKCAC+512, CYLS*512*18*2/4)
		MOV		ESI,DSKCAC0+512	; 源
		MOV		EDI,DSKCAC+512	; 目标
		MOV		ECX,0
		MOV		CL,BYTE [CYLS] 	;CYLS是柱面
		IMUL	ECX,512*18*2/4	; 除以4得到字节数
		SUB		ECX,512/4		; IPL偏移量
		CALL	memcpy

; 由于还需要asmhead才能完成
; 完成其余的bootpack任务

; bootpack启动
; 同样是memcpy
; 会将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000号地址去
		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]	;[EBX+16]:bootpack.hrb之后的第16号地址。值是0x11a8
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; 右移指令, ECX /= 4;
		JZ		skip			; 没有需要转送的东西时
		MOV		ESI,[EBX+20]	; 源 0x10c8
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 目标 0x00310000
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 堆栈的初始化, 将0x310000代入到ESP
		JMP		DWORD 2*8:0x0000001b ; 将2 * 8 代入到CS里，同时移动到第二个段的0x1b号地址,
		;第2个段的基地址是0x280000，所以实际上是从0x28001b开始执行的。这也就是bootpack.hrb的0x1b号地址

waitkbdout:
		IN		AL,0x64 		; 空读, 从设备号0x64读取数据
		AND		AL,0x02
		IN 		AL,0x60			; 空读, 从设备号0x60读取数据
		JNZ		waitkbdout		; AND结果不为0跳转到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 运算结果不为0跳转到memcpy
		RET
; memcpy地址前缀大小

		ALIGNB	16				;，一直添加DBO，直到时机合适的时候为止, 即能整除16为止
; GDT0也是一种特定的GDT, 0号为空区域
GDT0:
		RESB	8				; 空区域
		; set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段（segment）32bit
		; set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的文件的32bit寄存器（bootpack用）


		DW		0
; 相当于LGDT指令, 通知GDT0有GDT存在, 在GDT0里，写入了16位的段上限，和32位的段起始地址
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
