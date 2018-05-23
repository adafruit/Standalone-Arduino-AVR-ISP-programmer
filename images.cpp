#include "optiLoader.h"

const image_t PROGMEM image_328 = {
    // Sketch name, only used for serial printing
    {"optiboot_atmega328.hex"},
    // Chip name, only used for serial printing
    {"atmega328P"},
    // Signature bytes for 328P
    0x950F,
    // Programming fuses, written before writing to flash. Fuses set to
    // zero are untouched.
    {0x3F, 0xFF, 0xDE, 0x05}, // {lock, low, high, extended}
    // Normal fuses, written after writing to flash (but before
    // verifying). Fuses set to zero are untouched.
    {0x0F, 0x0, 0x0, 0x0}, // {lock, low, high, extended}
    // Fuse verify mask. Any bits set to zero in these values are
    // ignored while verifying the fuses after writing them. All (and
    // only) bits that are unused for this atmega chip should be zero
    // here.
    {0x3F, 0xFF, 0xFF, 0x07}, // {lock, low, high, extended}
    // size of chip flash in bytes
    32768,
    // size in bytes of flash page
    128,
    // The actual image to flash. This can be copy-pasted as-is from a
    // .hex file. If you do, replace all lines below starting with a
    // colon, but make sure to keep the start and end markers {R"( and
    // )"} in place.
    {R"(
:107E0000112494B714BE892F8D7011F0892FDED004
:107E100085E08093810082E08093C00088E18093B8
:107E2000C10086E08093C20080E18093C4008EE0B0
:107E3000B7D0259A86E020E33CEF91E030938500AF
:107E40002093840096BBB09BFECF1D9AA8958150CD
:107E5000A9F7EE24FF24B3E0AB2EBB24B394A5E036
:107E6000DA2EF1E1CF2E90D0813471F48DD0082F2D
:107E70009DD0023811F482E005C0013811F486E08B
:107E800001C083E079D075C0823411F484E103C06D
:107E9000853419F485E092D06CC0853579F474D0BE
:107EA000E82EFF2471D0082F10E0102F00270E2994
:107EB0001F29000F111F7AD078015BC0863521F48D
:107EC00084E07CD080E0DECF843609F035C05CD021
:107ED0005BD0182F59D0082FC0E0D1E055D089933E
:107EE0001C17E1F763D0053409F4FFCFF701A7BEF3
:107EF000E89507B600FCFDCFA701A0E0B1E02C910A
:107F000030E011968C91119790E0982F8827822B62
:107F1000932B1296FA010C01B7BEE89511244E5F1F
:107F20005F4F1A1761F7F701D7BEE89507B600FC57
:107F3000FDCFC7BEE8951DC0843769F425D024D095
:107F4000082F22D033D0E701FE018591EF0114D034
:107F50000150D1F70EC0853739F428D08EE10CD00E
:107F600085E90AD08FE08ECF813511F488E018D0F2
:107F70001DD080E101D077CF982F8091C00085FF80
:107F8000FCCF9093C60008958091C00087FFFCCF7E
:107F90008091C00084FD01C0A8958091C60008951D
:107FA000E0E6F0E098E1908380830895EDDF803291
:107FB00019F088E0F5DFFFCF84E1DECF1F93182FA3
:107FC000E3DF1150E9F7F2DF1F910895282E80E0DA
:0A7FD000E7DFEE27FF270994020601
:00000001FF
    )"}
};


/*
 * Table of defined images. The first one matching the chip's signature
 * is used.
 */
const image_t *images[] = {
  &image_328,
};

uint8_t NUMIMAGES = sizeof(images)/sizeof(images[0]);
