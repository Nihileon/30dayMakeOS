; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 制作目标文件的模式
[BITS 32]						; 制作32位模式用的机器语言
[INSTRSET "i486p"]

; 制作目标文件的信息

[FILE "naskfunc.nas"]			; 源文件名信息

		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt			; 程序中包含的函数名
		GLOBAL	_io_in8, _io_in16, _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL	_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		GLOBAL	_memtest_sub
		EXTERN	_inthandler21, _inthandler27, _inthandler2c

; 以下是实际的函数

[SECTION .text]		; 目标文件中写了这些后再写程序

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX, [ESP+4]	;PORT
		MOV		EAX, 0
		IN 		AL, DX
		RET

_io_in16:
		MOV		EDX, [ESP+4]	;PORT
		MOV		EAX, 0
		IN 		AX, DX
		RET

_io_in32:
		MOV		EDX, [ESP+4]
		IN 		EAX, DX
		RET

_io_out8:	; void _io_out8(int port, int data)
		MOV 	EDX, [ESP+4]
		MOV 	AL, [ESP+8]
		OUT 	DX,AL
		RET

_io_out16: ; void io_out16(int port, int data);
		MOV EDX,[ESP+4] ; port
		MOV EAX,[ESP+8] ; data
		OUT DX,AX
		RET
_io_out32: ; void io_out32(int port, int data);
		MOV EDX,[ESP+4] ; port
		MOV EAX,[ESP+8] ; data
		OUT DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD 		; means PUSH EDLAGS
		POP		EAX
		RET

_io_store_eflags:	; void _io_store_eflags
		MOV		EAX, [ESP+4]
		PUSH 	EAX
		POPFD
		RET

_load_gdtr: ;void _load_gdtr(int limit, int addr);
		MOV		AX,[ESP + 4]
		MOV		[ESP+6], AX
		LGDT 	[ESP+6]
		RET

_load_idtr: ;void _load_idtr(int limit, int addr)
		MOV		AX,[ESP+4]
		MOV		[ESP+6],AX
		LIDT 	[ESP+6]
		RET

_load_cr0: ; (void) -> void
		MOV		EAX, CR0
		RET

_store_cr0: ; (int cr0)->void
		MOV 	EAX, [ESP+4]
		MOV 	CR0, EAX
		RET

_asm_inthandler21: ; 在中断前保存寄存器状态, 并在中断后恢复寄存器
		PUSH 	ES
		PUSH 	DS
		PUSHAD 					; push EAX
		MOV 	EAX, ESP
		PUSH 	EAX
		MOV 	AX, SS
		MOV 	DS, AX
		MOV 	ES, AX
		CALL 	_inthandler21
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		IRETD

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_memtest_sub: ; (unsigned int start, unsigned int end)->unsigned int
		PUSH	EDI ; 需要使用这三个寄存器
		PUSH 	ESI
		PUSH 	EBX
		MOV		ESI, 0xAA55AA55 ; pat0 = 0xaa55aa55
		MOV 	EDI, 0x55AA55AA ; pat1 = 0x55aa55aa
		MOV 	EAX, [ESP+12+4] ; i = start
mts_loop:
		MOV 	EBX, EAX
		ADD 	EBX, 0xFFC		;p = i + 0xffc
		MOV		EDX, [EBX] 		;old = *p
		MOV 	[EBX], ESI 		;*p = pat0
		XOR 	DWORD [EBX], 0xFFFFFFFF ; *p^=0xffffffff
		CMP 	EDI, [EBX] 		; if(*p!=pat1) goto fin;
		JNE 	mts_fin
		XOR 	DWORD [EBX], 0xFFFFFFFF
		CMP 	ESI, [EBX] 		;if(*p != pat0) goto fin
		JNE 	mts_fin
		MOV 	[EBX], EDX 		;*p = old
		ADD 	EAX, 0x1000  	;i+=0x1000
		CMP 	EAX, [ESP+12+8]

		JBE 	mts_loop
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET
mts_fin:
		MOV 	[EBX], EDX 	 	;*p = old
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET
