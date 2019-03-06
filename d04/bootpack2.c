// 段描述符 64bit

struct SEGMENT_DESCRIPTOR{
    //0~15 16~31
    short limit_low, base_low;
    // 32~39 40~47
    char base_mid, access_right;
    // 48~55 56~63
    char limit_high, base_high;
};

// IDT的64bit内容
struct GATE_DESCRIPTOR
{
    //offset 段内偏移地址, selector 选择子
    // 0~15 16~31
    short offset_low, selector;
    // 32~39 40~ 47
    char dw_count, access_right;
    // 48~63
    short offset_high;
};

void init_gdt