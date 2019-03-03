; hello-os
; tab = 4

DB      0xeb, 0x4e, 0x90
DB      "HELLOIPL"          ;The booting section can be any string length of 8 byte.
DW      512                 ;Every sector MUST be 512 bytes in size.
DB      1                   ;Cluster MUST be 1 sector in size.
DW      1                   ;Starting position of FAT (It generally start from the 1st sector)
DB      2                   ;The number of FAT (MUST be 2)
DW      224                 ;The size of the root directory (generally 2)
DW      2880                ;The size of the disk (MUST be 2880 sectors)
DB      0xf0                ;The type of the disk (MUST be 0xf0)
DW      9                   ;The length of FAT (MUST be 9 sector)
DW      18                  ;The number of sector of a track (MUST be 18)
DW      2                   ;The number of magnetic head (MUST be 2)
DD      0                   ;NOT use partitions
DD      2880                ;the size when rewrite the disk
DB      0, 0, 0x29          ;meaningless
DD      0xffffffff          ;meaningless
DB      "HELLO-OS"          ;The name of disk (11 bytes)
DB      "FAT32"             ;The format of the disk (8 bytes)
RESB    18                  ;18 empty bytes

;the main body
DB        0xb8, 0x00, 0x00, 0x8e, 0xd0, 0xbc, 0x00, 0x7c
DB        0x8e, 0xd8, 0x8e, 0xc0, 0xbe, 0x74, 0x7c, 0x8a
DB        0x04, 0x83, 0xc6, 0x01, 0x3c, 0x00, 0x74, 0x09
DB        0xb4, 0x0e, 0xbb, 0x0f, 0x00, 0xcd, 0x10, 0xeb
DB        0xee, 0xf4, 0xeb, 0xfd

;display information
DB      0x0a, 0x0a               ;wrap twice
DB      "hello world"
DB      0x0a
DB      0

RESB 0x1fe-$                     ;write 0x00 until 0x001fe
DB 0x55, 0xaa

;output outside the booting section
DB      0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
RESB    4600
DB      0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
RESB    1469432