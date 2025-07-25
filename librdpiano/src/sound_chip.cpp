#include "../include/sound_chip.h"

#include <cmath>

// LUT for the address speed
static constexpr uint32_t env_table[] = {
    0x000000, 0x000023, 0x000026, 0x000029, 0x00002d, 0x000031, 0x000036,
    0x00003b, 0x000040, 0x000046, 0x00004c, 0x000052, 0x00005a, 0x000062,
    0x00006c, 0x000076, 0x000080, 0x00008c, 0x000098, 0x0000a4, 0x0000b4,
    0x0000c4, 0x0000d8, 0x0000ec, 0x000104, 0x00011c, 0x000134, 0x00014c,
    0x00016c, 0x00018c, 0x0001b4, 0x0001dc, 0x000200, 0x000230, 0x000260,
    0x000290, 0x0002d0, 0x000310, 0x000360, 0x0003b0, 0x000400, 0x000460,
    0x0004c0, 0x000520, 0x0005a0, 0x000620, 0x0006c0, 0x000760, 0x000800,
    0x0008c0, 0x000980, 0x000a40, 0x000b40, 0x000c40, 0x000d80, 0x000ec0,
    0x001000, 0x001180, 0x001300, 0x001480, 0x001680, 0x001880, 0x001b00,
    0x001d80, 0x002000, 0x002300, 0x002600, 0x002900, 0x002d00, 0x003100,
    0x003600, 0x003b00, 0x004000, 0x004600, 0x004c00, 0x005200, 0x005a00,
    0x006200, 0x006c00, 0x007600, 0x008000, 0x008c00, 0x009800, 0x00a400,
    0x00b400, 0x00c400, 0x00d800, 0x00ec00, 0x010000, 0x011800, 0x013000,
    0x014800, 0x016800, 0x018800, 0x01b000, 0x01d800, 0x020000, 0x023000,
    0x026000, 0x029000, 0x02d000, 0x031000, 0x036000, 0x03b000, 0x040000,
    0x046000, 0x04c000, 0x052000, 0x05a000, 0x062000, 0x06c000, 0x076000,
    0x080000, 0x08c000, 0x098000, 0x0a4000, 0x0b4000, 0x0c4000, 0x0d8000,
    0x0ec000, 0x100000, 0x118000, 0x130000, 0x148000, 0x168000, 0x188000,
    0x1b0000, 0x1d8000, 0x000000, 0x1fffdc, 0x1fffd9, 0x1fffd6, 0x1fffd2,
    0x1fffce, 0x1fffc9, 0x1fffc4, 0x1fffbf, 0x1fffb9, 0x1fffb3, 0x1fffad,
    0x1fffa5, 0x1fff9d, 0x1fff93, 0x1fff89, 0x1fff7f, 0x1fff73, 0x1fff67,
    0x1fff5b, 0x1fff4b, 0x1fff3b, 0x1fff27, 0x1fff13, 0x1ffefb, 0x1ffee3,
    0x1ffecb, 0x1ffeb3, 0x1ffe93, 0x1ffe73, 0x1ffe4b, 0x1ffe23, 0x1ffdff,
    0x1ffdcf, 0x1ffd9f, 0x1ffd6f, 0x1ffd2f, 0x1ffcef, 0x1ffc9f, 0x1ffc4f,
    0x1ffbff, 0x1ffb9f, 0x1ffb3f, 0x1ffadf, 0x1ffa5f, 0x1ff9df, 0x1ff93f,
    0x1ff89f, 0x1ff7ff, 0x1ff73f, 0x1ff67f, 0x1ff5bf, 0x1ff4bf, 0x1ff3bf,
    0x1ff27f, 0x1ff13f, 0x1fefff, 0x1fee7f, 0x1fecff, 0x1feb7f, 0x1fe97f,
    0x1fe77f, 0x1fe4ff, 0x1fe27f, 0x1fdfff, 0x1fdcff, 0x1fd9ff, 0x1fd6ff,
    0x1fd2ff, 0x1fceff, 0x1fc9ff, 0x1fc4ff, 0x1fbfff, 0x1fb9ff, 0x1fb3ff,
    0x1fadff, 0x1fa5ff, 0x1f9dff, 0x1f93ff, 0x1f89ff, 0x1f7fff, 0x1f73ff,
    0x1f67ff, 0x1f5bff, 0x1f4bff, 0x1f3bff, 0x1f27ff, 0x1f13ff, 0x1effff,
    0x1ee7ff, 0x1ecfff, 0x1eb7ff, 0x1e97ff, 0x1e77ff, 0x1e4fff, 0x1e27ff,
    0x1dffff, 0x1dcfff, 0x1d9fff, 0x1d6fff, 0x1d2fff, 0x1cefff, 0x1c9fff,
    0x1c4fff, 0x1bffff, 0x1b9fff, 0x1b3fff, 0x1adfff, 0x1a5fff, 0x19dfff,
    0x193fff, 0x189fff, 0x17ffff, 0x173fff, 0x167fff, 0x15bfff, 0x14bfff,
    0x13bfff, 0x127fff, 0x113fff, 0x0fffff, 0x0e7fff, 0x0cffff, 0x0b7fff,
    0x097fff, 0x077fff, 0x04ffff, 0x027fff};

// LUT for bits 5/6/7/8 of the subphase
static constexpr uint16_t addr_table[] = {0x1e0, 0x080, 0x060, 0x04d, 0x040, 0x036, 0x02d, 0x026,
                                          0x020, 0x01b, 0x016, 0x011, 0x00d, 0x00a, 0x006, 0x003};

#define UNSCRAMBLE_ADDR_WAVE(i) \
	((BIT(i, 16) << 16) | (BIT(i, 15) << 15) | (BIT(i, 14) << 14) | \
	(BIT(i, 1) << 13)   | (BIT(i, 4) << 12)  | (BIT(i, 9) << 11)  | \
	(BIT(i, 5) << 10)   | (BIT(i, 10) << 9)  | (BIT(i, 3) << 8)   | \
	(BIT(i, 0) << 7)    | (BIT(i, 6) << 6)   | (BIT(i, 11) << 5)  | \
	(BIT(i, 7) << 4)    | (BIT(i, 2) << 3)   | (BIT(i, 12) << 2)  | \
	(BIT(i, 8) << 1)    | (BIT(i, 13) << 0))
#define UNSCRAMBLE_DATA_WAVE(_data) \
	bitswap<8>(_data,7,6,5,4,3,2,1,0)

SoundChip::SoundChip(const u8 *temp_ic5, const u8 *temp_ic6, const u8 *temp_ic7)
{
    load_samples(temp_ic5, temp_ic6, temp_ic7);

    // Exp table to for the subphase
    // TODO: This is bit accurate, but I want to believe there is a better way to compute this function
    for (size_t i = 0; i < 0x10000; i++)
    {
        // ROM IC11
        uint16_t r11_pos = i % 4096;
        uint16_t r11 = (uint16_t)round(exp2f(13.0 + r11_pos / 4096.0) - 4096 * 2);
        bool r11_12 = !((r11 >> 12) & 1);
        bool r11_11 = !((r11 >> 11) & 1);
        bool r11_10 = !((r11 >> 10) & 1);
        bool r11_9 =  !((r11 >> 9) & 1);
        bool r11_8 =  !((r11 >> 8) & 1);
        bool r11_7 =  !((r11 >> 7) & 1);
        bool r11_6 =  !((r11 >> 6) & 1);
        bool r11_5 =  !((r11 >> 5) & 1);
        bool r11_4 =  (r11 >> 4) & 1;
        bool r11_3 =  (r11 >> 3) & 1;
        bool r11_2 =  (r11 >> 2) & 1;
        bool r11_1 =  (r11 >> 1) & 1;
        bool r11_0 =  (r11 >> 0) & 1;

        uint8_t param_bus_0 = ((i / 0x1000) >> 0) & 1;
        uint8_t param_bus_1 = ((i / 0x1000) >> 1) & 1;
        uint8_t param_bus_2 = ((i / 0x1000) >> 2) & 1;
        uint8_t param_bus_3 = ((i / 0x1000) >> 3) & 1;

        // Copy pasted from silicon
        bool result_b0 = (!r11_6 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_5 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_4 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_3 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_2 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_1 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_0 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3);
        bool result_b1 = (!r11_7 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_6 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_5 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_4 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_3 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_2 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_1 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (r11_0 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3);
        bool result_b2 = !(!((!r11_8 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_7 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_6 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_5 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (r11_4 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_3 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_2 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (r11_1 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !(r11_0 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b3 = !(!((!r11_9 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_8 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_7 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_6 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_5 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_4 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_3 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (r11_2 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((r11_1 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_0 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b4 = !(!((!r11_10 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_9 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_8 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_7 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_6 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_5 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (r11_4 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (r11_3 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((r11_2 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_1 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_0 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (0 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b5 = !(!((!r11_11 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_10 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_9 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_8 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_7 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_6 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_5 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (r11_4 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((r11_3 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_2 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_1 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (r11_0 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b6 = !(!((!r11_12 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_11 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_10 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_9 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_8 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_7 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_6 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_5 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((r11_4 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_3 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_2 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (r11_1 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b7 = !(!((1 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_12 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_11 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_10 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_9 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_8 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_7 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_6 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((!r11_5 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_4 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_3 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (r11_2 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b8 = !(!((0 && !param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (1 && param_bus_0 && !param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_12 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_11 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_10 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_9 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_8 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_7 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3)) && !((!r11_6 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_5 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (r11_4 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (r11_3 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b9 = !(!((1 && !param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_12 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_11 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_10 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_9 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_8 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_7 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_6 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3)) && !((!r11_5 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (r11_4 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)));
        bool result_b10 = !(!((1 && param_bus_0 && param_bus_1 && !param_bus_2 && !param_bus_3) || (!r11_12 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_11 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_10 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_9 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_8 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_7 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_6 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3)) && !(!r11_5 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b11 = (1 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_12 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_11 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_10 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_9 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_8 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_7 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_6 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3);
        bool result_b12 = (0 && !param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (1 && param_bus_0 && !param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_12 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_11 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_10 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_9 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_8 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_7 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3);
        bool result_b13 = (1 && !param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_12 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) || (!r11_11 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_10 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_9 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) || (!r11_8 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3);
        bool result_b14 = !(1 && !(1 && param_bus_0 && param_bus_1 && param_bus_2 && !param_bus_3) && !(!r11_12 && !param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_11 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_10 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_9 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b15 = !(!(!param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_12 && param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_11 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_10 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b16 = !(!(param_bus_0 && !param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_12 && !param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_11 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b17 = !(!(!param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3) && !(!r11_12 && param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3));
        bool result_b18 = param_bus_0 && param_bus_1 && !param_bus_2 && param_bus_3;
        
        uint32_t result =
            result_b18 << 18 | result_b17 << 17 | result_b16 << 16 | result_b15 << 15 | result_b14 << 14 | result_b13 << 13 |
            result_b12 << 12 | result_b11 << 11 | result_b10 << 10 | result_b9 << 9 | result_b8 << 8 | result_b7 << 7 |
            result_b6 << 6 | result_b5 << 5 | result_b4 << 4 | result_b3 << 3 | result_b2 << 2 | result_b1 << 1 | result_b0 << 0;
        phase_exp_table[i] = result;
    }

    // Exp table to decode samples
    // TODO: This is bit accurate, but I want to believe there is a better way to compute this function
    for (size_t i = 0; i < 0x8000; i++)
    {
        // ROM IC10
        uint16_t r10_pos = i % 1024;
        uint16_t r10 = (uint16_t)round(exp2f(11.0 + ~r10_pos / 1024.0) - 1024);
        bool r10_9 = (r10 >> 0) & 1;
        bool r10_8 = (r10 >> 1) & 1;
        bool r10_0 = (r10 >> 2) & 1;
        bool r10_1 = (r10 >> 3) & 1;
        bool r10_2 = (r10 >> 4) & 1;
        bool r10_3 = !((r10 >> 5) & 1);
        bool r10_4 = !((r10 >> 6) & 1);
        bool r10_5 = !((r10 >> 7) & 1);
        bool r10_6 = !((r10 >> 8) & 1);
        bool r10_7 = !((r10 >> 9) & 1);

        bool wavein_sign = i >= 0x4000;
        uint8_t add_r_0 = ((i / 0x400) >> 0) & 1;
        uint8_t add_r_1 = ((i / 0x400) >> 1) & 1;
        uint8_t add_r_2 = ((i / 0x400) >> 2) & 1;
        uint8_t add_r_3 = ((i / 0x400) >> 3) & 1;

        // Copy pasted from silicon
        bool result_b14 = !((!(!add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) && !wavein_sign) || (!add_r_3 && !add_r_2 && !add_r_1 && !add_r_0 && wavein_sign));
        bool result_b13 = !((((!r10_7 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && wavein_sign) || (!((!r10_7 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !wavein_sign));
        bool result_b12 = !((((!r10_6 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!add_r_3 && !add_r_2 && add_r_1 && !add_r_0)) && wavein_sign) || (!((!r10_6 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!add_r_3 && !add_r_2 && add_r_1 && !add_r_0)) && !wavein_sign));
        bool result_b11 = !((((!r10_5 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (1 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0)) && wavein_sign) || (!((!r10_5 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (1 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0)) && !wavein_sign));
        bool result_b10 = !((!((!r10_7 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !(!add_r_3 && add_r_2 && !add_r_1 && !add_r_0) && !wavein_sign) || (!(!((!r10_7 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !(!add_r_3 && add_r_2 && !add_r_1 && !add_r_0)) && wavein_sign));
        bool result_b9 = !((((1 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && wavein_sign) || (!((1 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !wavein_sign));
        bool result_b8 = !((((1 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (1 && 0)) && wavein_sign) || (!((1 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (1 && 0)) && !wavein_sign));
        bool result_b7 = !((((1 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && wavein_sign) || (!((1 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !wavein_sign));
        bool result_b6 = !((!((1 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !(r10_0 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) && !wavein_sign) || (!(!((1 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !(r10_0 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && wavein_sign));
        bool result_b5 = !((!((!r10_7 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !((r10_9 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !wavein_sign) || (!(!((!r10_7 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_6 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !((r10_9 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (add_r_3 && !add_r_2 && !add_r_1 && add_r_0))) && wavein_sign));
        bool result_b4 = !((!((r10_8 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_2 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (add_r_3 && !add_r_2 && add_r_1 && !add_r_0)) && !wavein_sign) || (!(!((r10_8 && !add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_2 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && !add_r_3 && add_r_2 && add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (add_r_3 && !add_r_2 && add_r_1 && !add_r_0))) && wavein_sign));
        bool result_b3 = !((!((r10_8 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_1 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (add_r_3 && !add_r_2 && add_r_1 && add_r_0)) && !wavein_sign) || (!(!((r10_8 && !add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_1 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_4 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (add_r_3 && !add_r_2 && add_r_1 && add_r_0))) && wavein_sign));
        bool result_b2 = !((!((r10_8 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_0 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_2 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (add_r_3 && add_r_2 && !add_r_1 && !add_r_0)) && !wavein_sign) || (!(!((r10_8 && !add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_0 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_2 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (!r10_3 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (add_r_3 && add_r_2 && !add_r_1 && !add_r_0))) && wavein_sign));
        bool result_b1 = !((!((r10_8 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_9 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_1 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_3 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_7 && add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (add_r_3 && add_r_2 && !add_r_1 && add_r_0)) && !wavein_sign) || (!(!((r10_8 && !add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (r10_9 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_0 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_1 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (r10_2 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (!r10_3 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0)) && !((!r10_6 && add_r_3 && !add_r_2 && add_r_1 && add_r_0) || (!r10_7 && add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (add_r_3 && add_r_2 && !add_r_1 && add_r_0))) && wavein_sign));
        bool result_b0 = !((!((r10_8 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_0 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (r10_2 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (add_r_3 && add_r_2 && add_r_1 && !add_r_0)) && !wavein_sign) || (!(!((r10_8 && !add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (r10_9 && !add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (r10_0 && !add_r_3 && add_r_2 && add_r_1 && !add_r_0) || (r10_1 && !add_r_3 && add_r_2 && add_r_1 && add_r_0) || (r10_2 && add_r_3 && !add_r_2 && !add_r_1 && !add_r_0) || (!r10_3 && add_r_3 && !add_r_2 && !add_r_1 && add_r_0) || (!r10_4 && add_r_3 && !add_r_2 && add_r_1 && !add_r_0) || (!r10_5 && add_r_3 && !add_r_2 && add_r_1 && add_r_0)) && !((!r10_6 && add_r_3 && add_r_2 && !add_r_1 && !add_r_0) || (!r10_7 && add_r_3 && add_r_2 && !add_r_1 && add_r_0) || (add_r_3 && add_r_2 && add_r_1 && !add_r_0))) && wavein_sign));
        
        uint16_t result =
            result_b14 << 14 | result_b13 << 13 | result_b12 << 12 | result_b11 << 11 | result_b10 << 10 |
            result_b9 << 9 | result_b8 << 8 | result_b7 << 7 | result_b6 << 6 | result_b5 << 5 |
            result_b4 << 4 | result_b3 << 3 | result_b2 << 2 | result_b1 << 1 | result_b0 << 0;
        samples_exp_table[i] = result;
    }
}

u8 SoundChip::read(size_t offset)
{
    return m_irq_id;
}

void SoundChip::write(size_t offset, u8 data)
{
    uint8_t voiceI = offset / 0x100;
    uint8_t partI = offset % 0x100 / 0x10;
    uint8_t field = offset % 8;

    if (voiceI >= NUM_VOICES || partI >= PARTS_PER_VOICE_MEM || field >= 8)
    {
        printf("ERROR: received invalid SA write %02x %02x %02x %02x\n", voiceI, partI, field, data);
        return;
    }

    SA_Part &part = m_parts[voiceI][partI];

    // flags seems to be common for all parts?
    if (field == 0x6)
    {
        m_parts[voiceI][0].flags_0 = data & 1;
        m_parts[voiceI][0].flags_1 = (data >> 1) & 1;
    }
    else if (field == 0x0)
    {
        part.pitch_lut_i &= 0x00FF;
        part.pitch_lut_i |= data << 8;
    }
    else if (field == 0x1)
    {
        part.pitch_lut_i &= 0xFF00;
        part.pitch_lut_i |= data;
    }
    else if (field == 0x2)
        part.wave_addr_loop = data;
    else if (field == 0x3)
        part.wave_addr_high = data;
    else if (field == 0x4)
        part.env_dest = data;
    else if (field == 0x5)
        part.env_speed = data;
    else if (field == 0x7)
        part.env_offset = data;
}

s32 SoundChip::update()
{
    s32 result = 0;
    
    for (size_t voiceI = 0; voiceI < NUM_VOICES; voiceI++)
    {
        SA_Part &partFlags = m_parts[voiceI][0];
        for (size_t partI = 0; partI < PARTS_PER_VOICE; partI++)
        {
            SA_Part &part = m_parts[voiceI][partI];

            // hack for performance
            if (part.env_value == 0 && part.env_dest == 0)
            {
                part.sub_phase = 0;
                continue;
            }

            bool irq = false;

            uint32_t volume;
            uint32_t waverom_addr;
            bool ag3_sel_sample_type;
            bool ag1_phase_hi;

            // IC19
            {
                bool env_speed_some_high = (part.env_speed & 0b01111111) != 0;
                bool env_speed_inv = (part.env_speed & 0b10000000) != 0;

                uint32_t adder1_a = part.env_value;
                if (!partFlags.flags_0)
                    adder1_a = 1 << 25;
                uint32_t adder1_b = env_table[part.env_speed];
                bool adder1_ci = env_speed_some_high && env_speed_inv;
                if (adder1_ci)
                    adder1_b |= 0x7f << 21;

                uint32_t adder3_o = 1 + (adder1_a >> 20) + part.env_offset;
                uint32_t adder3_of = adder3_o > 0xff;
                adder3_o &= 0xff;

                volume = ~(
                    (partFlags.flags_0 ? ((adder1_a >> 14) & 0b111111) : 0) |
                    ((adder3_o & 0b1111) << 6) |
                    (adder3_of ? ((adder3_o & 0b11110000) << 6) : 0)
                ) & 0x3fff;

                uint32_t adder1_o = adder1_a + adder1_b + (adder1_ci ? 1 : 0);
                uint32_t adder1_of = adder1_o > 0xfffffff;
                adder1_o &= 0xfffffff;

                uint32_t adder2_o = (adder1_o >> 20) + (~part.env_dest & 0xff) + 1;
                uint32_t adder2_of = adder2_o > 0xff;

                bool end_reached = env_speed_some_high && ((adder1_of != env_speed_inv) || (env_speed_inv != adder2_of));
                irq |= end_reached;

                part.env_value = end_reached ? (part.env_dest << 20) : adder1_o;
            }

            // IC9
            {
                uint32_t adder1 = (phase_exp_table[part.pitch_lut_i] + part.sub_phase) & 0xffffff;
                uint32_t adder2 = 1 + (adder1 >> 16) + ((~part.wave_addr_loop) & 0xff);
                bool adder2_co = adder2 > 0xff;
                adder2 &= 0xff;
                uint32_t adder1_and = !partFlags.flags_1 ? 0 : (adder1 & 0xffff);
                adder1_and |= (!partFlags.flags_1 ? 0 : (adder2_co ? adder2 : (adder1 >> 16))) << 16;

                part.sub_phase = adder1_and;
                waverom_addr = (part.wave_addr_high << 11) | ((part.sub_phase >> 9) & 0x7ff);

                ag3_sel_sample_type = BIT(waverom_addr, 16) || BIT(waverom_addr, 15) || BIT(waverom_addr, 14) ||
                                    !((BIT(waverom_addr, 13) && !BIT(waverom_addr, 11) && !BIT(waverom_addr, 12)) || !BIT(waverom_addr, 13));
                ag1_phase_hi = (
                    (BIT(part.pitch_lut_i, 15) && BIT(part.pitch_lut_i, 14)) ||
                    (BIT(part.sub_phase, 23) || BIT(part.sub_phase, 22) || BIT(part.sub_phase, 21) || BIT(part.sub_phase, 20)) ||
                    !partFlags.flags_1
                );
            }

            // IC8
            {
                uint32_t waverom_pa = samples_exp[waverom_addr];
                uint32_t waverom_pb = samples_delta[waverom_addr];
                bool sign_pa = samples_exp_sign[waverom_addr];
                bool sign_pb = samples_delta_sign[waverom_addr];
                waverom_pa |= ag3_sel_sample_type ? 1 : 0;
                waverom_pb |= ag3_sel_sample_type ? 0 : 1;

                if (ag1_phase_hi)
                    volume |= 0b1111 << 10;

                uint32_t tmp_1, tmp_2;

                uint32_t adder1_o = volume + waverom_pa;
                bool adder1_co = adder1_o > 0x3fff;
                adder1_o &= 0x3fff;
                if (adder1_co)
                    adder1_o |= 0x3c00;
                tmp_1 = adder1_o;

                uint32_t adder3_o = addr_table[(part.sub_phase >> 5) & 0xf] + (waverom_pb & 0x1ff);
                bool adder3_of = adder3_o > 0x1ff;
                adder3_o &= 0x1ff;
                if (adder3_of)
                    adder3_o |= 0x1e0;
                
                adder1_o = volume + (adder3_o << 5);
                adder1_co = adder1_o > 0x3fff;
                adder1_o &= 0x3fff;
                if (adder1_co)
                    adder1_o |= 0x3c00;
                tmp_2 = adder1_o;
                
                int32_t exp_val1 = samples_exp_table[(16384 * sign_pa) + (1024 * (tmp_1 >> 10)) + (tmp_1 & 1023)];
                int32_t exp_val2 = samples_exp_table[(16384 * sign_pb) + (1024 * (tmp_2 >> 10)) + (tmp_2 & 1023)];
                if (sign_pa)
                    exp_val1 = exp_val1 - 0x8000;
                if (sign_pb)
                    exp_val2 = exp_val2 - 0x8000;
                int32_t exp_val = exp_val1 + exp_val2;
                
                // hack to prevent voices ringing when env value is 0, investigate
                if (part.env_value != 0)
                    result += exp_val;
            }

            if (irq && !m_irq_triggered)
            {
                m_irq_id = partI | (voiceI << 4);
                m_irq_triggered = true;
            }
        }
    }

    return result;
}

void SoundChip::load_samples(const u8 *temp_ic5, const u8 *temp_ic6, const u8 *temp_ic7)
{
    u8 ic5[0x20000];
    u8 ic6[0x20000];
    u8 ic7[0x20000];
    for (size_t srcpos = 0x00; srcpos < 0x20000; srcpos++) {
		ic5[srcpos] = UNSCRAMBLE_DATA_WAVE(temp_ic5[UNSCRAMBLE_ADDR_WAVE(srcpos)]);
	}
	for (size_t srcpos = 0x00; srcpos < 0x20000; srcpos++) {
		ic6[srcpos] = UNSCRAMBLE_DATA_WAVE(temp_ic6[UNSCRAMBLE_ADDR_WAVE(srcpos)]);
	}
	for (size_t srcpos = 0x00; srcpos < 0x20000; srcpos++) {
		ic7[srcpos] = UNSCRAMBLE_DATA_WAVE(temp_ic7[UNSCRAMBLE_ADDR_WAVE(srcpos)]);
	}

    // Wave rom values
    for (size_t i = 0; i < 0x20000; i++)
    {
        size_t descrambled_i = (
            ((i >> 0) & 1) << 0 |
            ((~i >> 1) & 1) << 1 |
            ((i >> 2) & 1) << 2 |
            ((~i >> 3) & 1) << 3 |
            ((i >> 4) & 1) << 4 |
            ((~i >> 5) & 1) << 5 |
            ((i >> 6) & 1) << 6 |
            ((i >> 7) & 1) << 7 |
            ((~i >> 8) & 1) << 8 |
            ((~i >> 9) & 1) << 9 |
            ((i >> 10) & 1) << 10 |
            ((i >> 11) & 1) << 11 |
            ((i >> 12) & 1) << 12 |
            ((i >> 13) & 1) << 13 |
            ((i >> 14) & 1) << 14 |
            ((i >> 15) & 1) << 15 |
            ((i >> 16) & 1) << 16
        );

        uint16_t exp_sample = (
            ((ic5[descrambled_i] >> 0) & 1) << 13 |
            ((ic6[descrambled_i] >> 4) & 1) << 12 |
            ((ic7[descrambled_i] >> 4) & 1) << 11 |
            ((~ic6[descrambled_i] >> 0) & 1) << 10 |
            ((ic7[descrambled_i] >> 7) & 1) << 9 |
            ((ic5[descrambled_i] >> 7) & 1) << 8 |
            ((~ic5[descrambled_i] >> 5) & 1) << 7 |
            ((ic6[descrambled_i] >> 2) & 1) << 6 |
            ((ic7[descrambled_i] >> 2) & 1) << 5 |
            ((ic7[descrambled_i] >> 1) & 1) << 4 |
            ((~ic5[descrambled_i] >> 1) & 1) << 3 |
            ((ic5[descrambled_i] >> 3) & 1) << 2 |
            ((ic6[descrambled_i] >> 5) & 1) << 1 |
            ((~ic6[descrambled_i] >> 7) & 1) << 0
        );
        bool exp_sign = (~ic7[descrambled_i] >> 3) & 1;
        samples_exp[i] = exp_sample;
        samples_exp_sign[i] = exp_sign;

        uint16_t delta_sample = (
            ((~ic7[descrambled_i] >> 6) & 1) << 8 |
            ((ic5[descrambled_i] >> 4) & 1) << 7 |
            ((ic7[descrambled_i] >> 0) & 1) << 6 |
            ((~ic6[descrambled_i] >> 3) & 1) << 5 |
            ((ic5[descrambled_i] >> 2) & 1) << 4 |
            ((~ic5[descrambled_i] >> 6) & 1) << 3 |
            ((ic6[descrambled_i] >> 6) & 1) << 2 |
            ((ic7[descrambled_i] >> 5) & 1) << 1 |
            ((~ic6[descrambled_i] >> 7) & 1) << 0
        );
        bool delta_sign = (ic6[descrambled_i] >> 1) & 1;
        samples_delta[i] = delta_sample;
        samples_delta_sign[i] = delta_sign;
    }
}
