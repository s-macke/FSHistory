#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../utils/exit_strategy.h"

#include "../cpu/cpu.h"
#include "fonts.h"
#include "screen.h"
#include "ram.h"

static uint8_t *vgaram = 0;

/*
  https://www-user.tu-chemnitz.de/~kzs/tools/whatvga/vga.txt
  https://en.wikipedia.org/wiki/Hercules_Graphics_Card
  http://www.tinyvga.com/6845
  https://en.wikipedia.org/wiki/Motorola_6845
  http://stanislavs.org/helppc/ports.html
  http://www.clannad.co.uk/datasheets/computer/pcintern.pdf
  http://minuszerodegrees.net/

3B0-3BF Monochrome Display Adapter (write only, see ~6845~)
    3B0 port address decodes to 3B4
    3B1 port address decodes to 3B5
    3B2 port address decodes to 3B4
    3B3 port address decodes to 3B5
    3B4 6845 index register, selects which register [0-11h]
        is to be accessed through port 3B5
    3B5 6845 data register [0-11h] selected by port 3B4,
        registers 0C-0F may be read.  If a read occurs without
        the adapter installed, FFh is returned.  (see 6845)
    3B6 port address decodes to 3B4
    3B7 port address decodes to 3B5
    3B8 6845 Mode control register
    3B9 reserved for color select register on color adapter
    3BA status register (read only)
    3BB reserved for light pen strobe reset

3C0-3CF  EGA/VGA
    3C0 VGA attribute and sequencer register
    3C1 Other video attributes
    3C2 EGA, VGA, CGA input status 0
    3C3 Video subsystem enable
    3C4 CGA, EGA, VGA sequencer index
    3C5 CGA, EGA, VGA sequencer
    3C6 VGA video DAC PEL mask
    3C7 VGA video DAC state
    3C8 VGA video DAC PEL address
    3C9 VGA video DAC
    3CA VGA graphics 2 position
    3CC VGA graphics 1 position
    3CD VGA feature control
    3CE VGA graphics index
    3CF Other VGA graphics

3D0-3DF Color Graphics Monitor Adapter (ports 3D0-3DB are write only, see 6845)
    3D0 port address decodes to 3D4
    3D1 port address decodes to 3D5
    3D2 port address decodes to 3D4
    3D3 port address decodes to 3D5
    3D4 6845 index register, selects which register [0-11h]
        is to be accessed through port 3D5
    3D5 6845 data register [0-11h] selected by port 3D4,
        registers 0C-0F may be read.  If a read occurs without
        the adapter installed, FFh is returned.  (see 6845)
    3D6 port address decodes to 3D4
    3D7 port address decodes to 3D5
    3D8 6845 Mode control register (CGA, EGA, VGA, except PCjr)
    3D9 color select palette register (CGA, EGA, VGA, see 6845)
    3DA status register (read only, see 6845, PCjr VGA access)
    3DB Clear light pen latch (any write)
    3DC Preset Light pen latch
    3DF CRT/CPU page register (PCjr only)

*/
// http://www.cns.utoronto.ca/~pkern/stuff/cterm+/cterm/int-10

//https://github.com/joncampbell123/dosbox-x/blob/master/src/ints/int10_modes.cpp

static uint8_t vga_palette[248][3] =
        {
                {0x00, 0x00, 0x00},
                {0x00, 0x00, 0x2a},
                {0x00, 0x2a, 0x00},
                {0x00, 0x2a, 0x2a},
                {0x2a, 0x00, 0x00},
                {0x2a, 0x00, 0x2a},
                {0x2a, 0x15, 0x00},
                {0x2a, 0x2a, 0x2a},
                {0x15, 0x15, 0x15},
                {0x15, 0x15, 0x3f},
                {0x15, 0x3f, 0x15},
                {0x15, 0x3f, 0x3f},
                {0x3f, 0x15, 0x15},
                {0x3f, 0x15, 0x3f},
                {0x3f, 0x3f, 0x15},
                {0x3f, 0x3f, 0x3f},
                {0x00, 0x00, 0x00},
                {0x05, 0x05, 0x05},
                {0x08, 0x08, 0x08},
                {0x0b, 0x0b, 0x0b},
                {0x0e, 0x0e, 0x0e},
                {0x11, 0x11, 0x11},
                {0x14, 0x14, 0x14},
                {0x18, 0x18, 0x18},
                {0x1c, 0x1c, 0x1c},
                {0x20, 0x20, 0x20},
                {0x24, 0x24, 0x24},
                {0x28, 0x28, 0x28},
                {0x2d, 0x2d, 0x2d},
                {0x32, 0x32, 0x32},
                {0x38, 0x38, 0x38},
                {0x3f, 0x3f, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x10, 0x00, 0x3f},
                {0x1f, 0x00, 0x3f},
                {0x2f, 0x00, 0x3f},
                {0x3f, 0x00, 0x3f},
                {0x3f, 0x00, 0x2f},
                {0x3f, 0x00, 0x1f},
                {0x3f, 0x00, 0x10},
                {0x3f, 0x00, 0x00},
                {0x3f, 0x10, 0x00},
                {0x3f, 0x1f, 0x00},
                {0x3f, 0x2f, 0x00},
                {0x3f, 0x3f, 0x00},
                {0x2f, 0x3f, 0x00},
                {0x1f, 0x3f, 0x00},
                {0x10, 0x3f, 0x00},
                {0x00, 0x3f, 0x00},
                {0x00, 0x3f, 0x10},
                {0x00, 0x3f, 0x1f},
                {0x00, 0x3f, 0x2f},
                {0x00, 0x3f, 0x3f},
                {0x00, 0x2f, 0x3f},
                {0x00, 0x1f, 0x3f},
                {0x00, 0x10, 0x3f},
                {0x1f, 0x1f, 0x3f},
                {0x27, 0x1f, 0x3f},
                {0x2f, 0x1f, 0x3f},
                {0x37, 0x1f, 0x3f},
                {0x3f, 0x1f, 0x3f},
                {0x3f, 0x1f, 0x37},
                {0x3f, 0x1f, 0x2f},
                {0x3f, 0x1f, 0x27},

                {0x3f, 0x1f, 0x1f},
                {0x3f, 0x27, 0x1f},
                {0x3f, 0x2f, 0x1f},
                {0x3f, 0x37, 0x1f},
                {0x3f, 0x3f, 0x1f},
                {0x37, 0x3f, 0x1f},
                {0x2f, 0x3f, 0x1f},
                {0x27, 0x3f, 0x1f},
                {0x1f, 0x3f, 0x1f},
                {0x1f, 0x3f, 0x27},
                {0x1f, 0x3f, 0x2f},
                {0x1f, 0x3f, 0x37},
                {0x1f, 0x3f, 0x3f},
                {0x1f, 0x37, 0x3f},
                {0x1f, 0x2f, 0x3f},
                {0x1f, 0x27, 0x3f},
                {0x2d, 0x2d, 0x3f},
                {0x31, 0x2d, 0x3f},
                {0x36, 0x2d, 0x3f},
                {0x3a, 0x2d, 0x3f},
                {0x3f, 0x2d, 0x3f},
                {0x3f, 0x2d, 0x3a},
                {0x3f, 0x2d, 0x36},
                {0x3f, 0x2d, 0x31},
                {0x3f, 0x2d, 0x2d},
                {0x3f, 0x31, 0x2d},
                {0x3f, 0x36, 0x2d},
                {0x3f, 0x3a, 0x2d},
                {0x3f, 0x3f, 0x2d},
                {0x3a, 0x3f, 0x2d},
                {0x36, 0x3f, 0x2d},
                {0x31, 0x3f, 0x2d},
                {0x2d, 0x3f, 0x2d},
                {0x2d, 0x3f, 0x31},
                {0x2d, 0x3f, 0x36},
                {0x2d, 0x3f, 0x3a},
                {0x2d, 0x3f, 0x3f},
                {0x2d, 0x3a, 0x3f},
                {0x2d, 0x36, 0x3f},
                {0x2d, 0x31, 0x3f},
                {0x00, 0x00, 0x1c},
                {0x07, 0x00, 0x1c},
                {0x0e, 0x00, 0x1c},
                {0x15, 0x00, 0x1c},
                {0x1c, 0x00, 0x1c},
                {0x1c, 0x00, 0x15},
                {0x1c, 0x00, 0x0e},
                {0x1c, 0x00, 0x07},
                {0x1c, 0x00, 0x00},
                {0x1c, 0x07, 0x00},
                {0x1c, 0x0e, 0x00},
                {0x1c, 0x15, 0x00},
                {0x1c, 0x1c, 0x00},
                {0x15, 0x1c, 0x00},
                {0x0e, 0x1c, 0x00},
                {0x07, 0x1c, 0x00},
                {0x00, 0x1c, 0x00},
                {0x00, 0x1c, 0x07},
                {0x00, 0x1c, 0x0e},
                {0x00, 0x1c, 0x15},
                {0x00, 0x1c, 0x1c},
                {0x00, 0x15, 0x1c},
                {0x00, 0x0e, 0x1c},
                {0x00, 0x07, 0x1c},

                {0x0e, 0x0e, 0x1c},
                {0x11, 0x0e, 0x1c},
                {0x15, 0x0e, 0x1c},
                {0x18, 0x0e, 0x1c},
                {0x1c, 0x0e, 0x1c},
                {0x1c, 0x0e, 0x18},
                {0x1c, 0x0e, 0x15},
                {0x1c, 0x0e, 0x11},
                {0x1c, 0x0e, 0x0e},
                {0x1c, 0x11, 0x0e},
                {0x1c, 0x15, 0x0e},
                {0x1c, 0x18, 0x0e},
                {0x1c, 0x1c, 0x0e},
                {0x18, 0x1c, 0x0e},
                {0x15, 0x1c, 0x0e},
                {0x11, 0x1c, 0x0e},
                {0x0e, 0x1c, 0x0e},
                {0x0e, 0x1c, 0x11},
                {0x0e, 0x1c, 0x15},
                {0x0e, 0x1c, 0x18},
                {0x0e, 0x1c, 0x1c},
                {0x0e, 0x18, 0x1c},
                {0x0e, 0x15, 0x1c},
                {0x0e, 0x11, 0x1c},
                {0x14, 0x14, 0x1c},
                {0x16, 0x14, 0x1c},
                {0x18, 0x14, 0x1c},
                {0x1a, 0x14, 0x1c},
                {0x1c, 0x14, 0x1c},
                {0x1c, 0x14, 0x1a},
                {0x1c, 0x14, 0x18},
                {0x1c, 0x14, 0x16},
                {0x1c, 0x14, 0x14},
                {0x1c, 0x16, 0x14},
                {0x1c, 0x18, 0x14},
                {0x1c, 0x1a, 0x14},
                {0x1c, 0x1c, 0x14},
                {0x1a, 0x1c, 0x14},
                {0x18, 0x1c, 0x14},
                {0x16, 0x1c, 0x14},
                {0x14, 0x1c, 0x14},
                {0x14, 0x1c, 0x16},
                {0x14, 0x1c, 0x18},
                {0x14, 0x1c, 0x1a},
                {0x14, 0x1c, 0x1c},
                {0x14, 0x1a, 0x1c},
                {0x14, 0x18, 0x1c},
                {0x14, 0x16, 0x1c},
                {0x00, 0x00, 0x10},
                {0x04, 0x00, 0x10},
                {0x08, 0x00, 0x10},
                {0x0c, 0x00, 0x10},
                {0x10, 0x00, 0x10},
                {0x10, 0x00, 0x0c},
                {0x10, 0x00, 0x08},
                {0x10, 0x00, 0x04},
                {0x10, 0x00, 0x00},
                {0x10, 0x04, 0x00},
                {0x10, 0x08, 0x00},
                {0x10, 0x0c, 0x00},
                {0x10, 0x10, 0x00},
                {0x0c, 0x10, 0x00},
                {0x08, 0x10, 0x00},
                {0x04, 0x10, 0x00},

                {0x00, 0x10, 0x00},
                {0x00, 0x10, 0x04},
                {0x00, 0x10, 0x08},
                {0x00, 0x10, 0x0c},
                {0x00, 0x10, 0x10},
                {0x00, 0x0c, 0x10},
                {0x00, 0x08, 0x10},
                {0x00, 0x04, 0x10},
                {0x08, 0x08, 0x10},
                {0x0a, 0x08, 0x10},
                {0x0c, 0x08, 0x10},
                {0x0e, 0x08, 0x10},
                {0x10, 0x08, 0x10},
                {0x10, 0x08, 0x0e},
                {0x10, 0x08, 0x0c},
                {0x10, 0x08, 0x0a},
                {0x10, 0x08, 0x08},
                {0x10, 0x0a, 0x08},
                {0x10, 0x0c, 0x08},
                {0x10, 0x0e, 0x08},
                {0x10, 0x10, 0x08},
                {0x0e, 0x10, 0x08},
                {0x0c, 0x10, 0x08},
                {0x0a, 0x10, 0x08},
                {0x08, 0x10, 0x08},
                {0x08, 0x10, 0x0a},
                {0x08, 0x10, 0x0c},
                {0x08, 0x10, 0x0e},
                {0x08, 0x10, 0x10},
                {0x08, 0x0e, 0x10},
                {0x08, 0x0c, 0x10},
                {0x08, 0x0a, 0x10},
                {0x0b, 0x0b, 0x10},
                {0x0c, 0x0b, 0x10},
                {0x0d, 0x0b, 0x10},
                {0x0f, 0x0b, 0x10},
                {0x10, 0x0b, 0x10},
                {0x10, 0x0b, 0x0f},
                {0x10, 0x0b, 0x0d},
                {0x10, 0x0b, 0x0c},
                {0x10, 0x0b, 0x0b},
                {0x10, 0x0c, 0x0b},
                {0x10, 0x0d, 0x0b},
                {0x10, 0x0f, 0x0b},
                {0x10, 0x10, 0x0b},
                {0x0f, 0x10, 0x0b},
                {0x0d, 0x10, 0x0b},
                {0x0c, 0x10, 0x0b},
                {0x0b, 0x10, 0x0b},
                {0x0b, 0x10, 0x0c},
                {0x0b, 0x10, 0x0d},
                {0x0b, 0x10, 0x0f},
                {0x0b, 0x10, 0x10},
                {0x0b, 0x0f, 0x10},
                {0x0b, 0x0d, 0x10},
                {0x0b, 0x0c, 0x10}
        };

uint32_t palette[256];

uint16_t vidmode = 3;
uint32_t vidmemory = 0x18000;

uint8_t monochrome_mode = 0;
uint8_t active_ega_reg = 0;
uint8_t ega_bitmask = 0xFF;
uint8_t ega_mode = 0x0;
uint8_t ega_function = 0x0;
uint8_t ega_enable_setreset = 0xFF;
uint8_t ega_setreset = 0xFF;
uint8_t ega_color_compare = 0x0;
uint8_t ega_color_dont_care = 0x0;
uint8_t ega_read_map_select = 0x0;
uint8_t active_ega_sequencer_reg = 0x0;
uint8_t ega_sequencer_bitplanemask = 0x0F;
uint8_t active_crt_reg = 0;


int cursor_column = 0;
int cursor_row = 0;

uint32_t VGA_GetVideoMode() {
    return vidmode;
}

/*
void DrawChar(uint8_t x)
{
    for(int j=0; j<16; j++)
    for(int i=0; i<8; i++)
        pixels[SCREEN_WIDTH*(j+cursor_row*16) + (i+cursor_column*8)] = 0xFF000000 | (int10_font_16[(x<<4) | j] & (1<<(7-i))?-1:0);
}
*/

void HandleCharGenerator() {
    uint8_t ah = regs.byteregs[regah];
    uint8_t al = regs.byteregs[regal];
    switch (al) {
        /*
        case 0x00: // set cursor position or set video mode???
        {
            uint8_t dh = regs.byteregs[regdh];
            uint8_t dl = regs.byteregs[regdl];
            uint8_t bl = regs.byteregs[regbl];
            printf("set cursor position: row:%i column:%i pagenumver:%i\n", dh, dl, bl);
            exit_or_restart(1);
        }
        break;
    */
        case 0x30: // get stats information about the char generator
        {
            // https://de.wikibooks.org/wiki/Interrupts_80x86/_INT_10#Funktion_10h:_Farbeinstellungen_(EGA/VGA)
            uint8_t requestedInformation = regs.byteregs[regbh];
            printf("requested information: %i\n", requestedInformation);
            regs.wordregs[regcx] = 16; // height of the matrix
            regs.byteregs[regdl] = 24; // columns-1
            segregs[reges] = 0xC000; // VGA bios
            regs.wordregs[regbp] = 0x8500; // VGA bios
            break;
        }

        default:
            printf("Error: Unknown VGA char function\n");
            exit_or_restart(1);
            break;
    }
}

void SetPalette() {
    uint8_t ah = regs.byteregs[regah];
    uint8_t al = regs.byteregs[regal];

    switch (al) {

        case 0x0: // set individual palette register
        {
            uint8_t value = regs.byteregs[regbh];
            uint8_t reg = regs.byteregs[regbl];
            //printf("set palette entry %i: %i\n", reg, value);
            if (value > 63) {
                printf("Error: Palette value too large\n");
                exit_or_restart(1);
            }
            if (reg > 16) {
                printf("Error: Palette index too large\n");
                exit_or_restart(1);
            }

            uint32_t r = 85 * (((value >> 1) & 2) | ((value >> 5) & 1));
            uint32_t g = 85 * ((value & 2) | ((value >> 4) & 1));
            uint32_t b = 85 * (((value << 1) & 2) | ((value >> 3) & 1));
            palette[reg] = 0xFF000000 | (b << 18) | (g << 10) | (r << 2);
        }
            break;

        case 0x2: // set all palettes and border TODO
            for (int i = 0; i < 17; i++) {
                uint8_t c = Read8Long(segregs[reges], regs.wordregs[regdx] + i);
                if (c > 63) {
                    printf("Error: Palette value too large\n");
                    exit_or_restart(1);
                }
                uint32_t r = 85 * (((c >> 1) & 2) | ((c >> 5) & 1));
                uint32_t g = 85 * ((c & 2) | ((c >> 4) & 1));
                uint32_t b = 85 * (((c << 1) & 2) | ((c >> 3) & 1));
                palette[i] = 0xFF000000 | (b << 18) | (g << 10) | (r << 2);
            }
            break;

        case 0x12: // load ramdac colors
        {
            int from = regs.wordregs[regbx];
            int count = regs.wordregs[regcx];
            //printf("Load colors from %i, count %i\n", from, count);

            for (int i = 0; i < count; i++) {
                uint32_t r = Read8Long(segregs[reges], regs.wordregs[regdx] + i * 3 + 0);
                uint32_t g = Read8Long(segregs[reges], regs.wordregs[regdx] + i * 3 + 1);
                uint32_t b = Read8Long(segregs[reges], regs.wordregs[regdx] + i * 3 + 2);
                palette[(from + i) & 0xFF] = 0xFF000000 | (b << 18) | (g << 10) | (r << 2);
            }
            break;
        }

        default:
            printf("Error: Unknown VGA color function\n");
            exit_or_restart(1);
            break;
    }
}

void VGA_Bios() {
    uint8_t ah = regs.byteregs[regah];
    uint8_t al = regs.byteregs[regal];
    //printf("VGA Bios ah: 0x%02x al: 0x%02x\n", ah, al);

    switch (ah) {

        case 0x0: // set video mode
            printf("set video mode: 0x%02x\n", al);
            vidmode = al;
            ram[0x449] = al; // current video mode in bios data area http://stanislavs.org/helppc/bios_data_area.html

            if (al == 0x00) vidmemory = 0x18000; // 0xb8000
            if (al == 0x02) vidmemory = 0x18000;
            if (al == 0x03) vidmemory = 0x18000;
            if (al == 0x04) vidmemory = 0x18000;
            if (al == 0x05) vidmemory = 0x18000;
            if (al == 0x06) vidmemory = 0x18000;
            if (al == 0x07) vidmemory = 0x10000;
            if (al == 0x0d) vidmemory = 0x18000; // 0xa0000
            //if (al == 0x10) vidmemory = 0x08000*8;
            if (al == 0x10) vidmemory = 0x00000;
            if (al == 0x13) vidmemory = 0x00000;
            if (al != 0x00 && al != 0x02 && al != 0x03 && al != 0x07 && al != 0x13 && al != 0x10 && al != 0x04 &&
                al != 0x0d && al != 0x06 && al != 0x05) {
                printf("unknown video mode 0x%02x\n", al);
                exit_or_restart(1);
            }
            //exit_or_restart(1);
            break;

        case 0x02: // set cursor position
        {
            cursor_row = regs.byteregs[regdh];
            cursor_column = regs.byteregs[regdl];
            uint8_t page = regs.byteregs[regbh];
            //printf("set cursor position: row:%i column:%i pagenumver:%i\n", cursor_row, cursor_column, page);
            break;
        }

        case 0x03: // get cursor position
        {
            regs.byteregs[regdh] = cursor_row;
            regs.byteregs[regdl] = cursor_column;
            regs.byteregs[regch] = 0;
            regs.byteregs[regcl] = 0;
            //printf("get cursor position\n");
            break;
        }

        case 0x0b: // set color palette http://stanislavs.org/helppc/int_10-b.html
            printf("set color palette: id:%i value:%i\n", regs.byteregs[regbh], regs.byteregs[regbl]);
            break;

        case 0x0e: // write text in teletype mode
        {
            uint8_t c = regs.byteregs[regal];
            if (c == 0x0D) cursor_column = 0; // carriage return
            if (c == 0x0A) cursor_row++; // line feed
            if (c >= 0x20) {
                vgaram[vidmemory + cursor_row * 160 + cursor_column * 2] = c;
                cursor_column++;
                if (cursor_column >= 80) {
                    cursor_column = 0;
                    cursor_row++;
                }
            }
            //printf("write text: %c\n", regs.byteregs[regal]);
            //exit_or_restart(1);
            break;
        }

        case 0x06: // scroll up window
            printf("scroll up window\n");
            break;

        case 0x8: // read attribute and char at position
            regs.byteregs[regal] = 0;
            regs.byteregs[regbl] = 0; // columns
            regs.byteregs[regbh] = 0; // isactive page
            //exit_or_restart(1);
            break;

        case 0x10: // colors
            SetPalette();
            break;

        case 0x11: // char generator
            HandleCharGenerator();
            break;

            /*
            case 0x9: // Write character and attribute at cursor position
                printf("al: 0x%02x\n", al);
                exit_or_restart(1);
                break;
            */
        case 0xf: // get video mode
            regs.byteregs[regal] = vidmode; // video mode
            regs.byteregs[regah] = 80; // columns
            regs.byteregs[regbh] = 0; // isactive page
            //exit_or_restart(1);
            break;

        case 0x4f: // vesa https://de.wikibooks.org/wiki/Interrupts_80x86/_INT_10#Funktion_4Fh:_VESA-Schnittstelle_(VBE)
                   // https://www.lowlevel.eu/wiki/VESA_BIOS_Extensions
                   // AL == 4Fh:      Function is supported
                   // Al != 4Fh:      Function is not supported
                   // AH == 00h:      Function call successful
                   // AH == 01h:      Function call failed
            switch(al) {
                case 0x01: // Information about VESA mode
                {
                    uint16_t cx = regs.wordregs[regcx];
                    uint16_t es = segregs[reges];
                    uint16_t di = regs.wordregs[regdi];
                    printf("information about vesa mode 0x%04x es:di=0x%04x:0x%04x\n", cx, es, di);
                    regs.wordregs[regax] = 0x4f;// function is supported
                    Write16Long(es, di + 0x00, 0x9b); // mode
                    Write8Long (es, di + 0x02, 7); // window available and read write
                    Write8Long (es, di + 0x03, 7); // window available and read write
                    Write16Long(es, di + 0x04, 0x40); // granularity in kilobytes to move the window
                    Write16Long(es, di + 0x06, 128); // window size 128kb
                    Write16Long(es, di + 0x08, 0xA000);
                    Write16Long(es, di + 0x0A, 0xB000);
                    Write16Long(es, di + 0x0C, 0x1320); // dosbox compabilitiy, pointer to the window function
                    //Write16Long(es, di + 0x0C, 0x0004); // offset of function to change window // this should fail
                    Write16Long(es, di + 0x0E, 0xF000); // segment of function to change window
                    Write16Long(es, di + 0x10, 640);
                    Write16Long(es, di + 0x12, 640);
                    Write16Long(es, di + 0x14, cx==0x100?400:480);
                    Write8Long (es, di + 0x18, 1); // number of bitplanes
                    Write8Long (es, di + 0x19, 8); // bits per pixel
                    Write8Long (es, di + 0x1b, 4); // memory modell, see https://www.lowlevel.eu/wiki/VESA_BIOS_Extensions

                    Write8Long(0xF000, 0x1320, 0x63); // rome interrupt 0x81
                    Write8Long(0xF000, 0x1321, 0x81);
                    Write8Long(0xF000, 0x1322, 0xCB); // retf

                    break;
                }
                case 0x02: // vesa on
                {
                    uint16_t bx = regs.wordregs[regbx];
                    vidmode = bx;
                    printf("vesa: turn on mode 0x%04x\n", bx);
                    regs.wordregs[regax] = 0x4f;// function is supported
                }
                break;

                default:
                    printf("Error: Unknown VGA VESA function 0x%02x\n", al);
                    exit_or_restart(1);
                    break;
            }
            break;

        default:
            printf("Error: Unknown VGA function 0x%02x\n", ah);
            exit_or_restart(1);
            break;
    }
}

int moderegister = 0;

uint8_t VGA_in(uint16_t portnum) {
    //printf("VGA in 0x%04x\n", portnum);

    switch (portnum) {

        case 0x3c7: // VGA video DAC state
            return 0x0;
            // TODO
            break;


        case 0x3ba: // 0x3ba, status register (read only)
            return 0;

        case 0x3da: // 0x3da, status register (read only) http://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Color%20Graphics%20Monitor%20Adapter%20(CGA).pdf
            moderegister ^= 1 << 3;
            //return 1<<3; // is in screen retrace mode
            return moderegister; // is not in screen retrace mode

        default:
            printf("Error Unknown in VGA port\n");
            exit_or_restart(1);
            return 0;
    }
}

void VGA_out16(uint16_t portnum, uint16_t value) {
    //printf("VGA out16 0x%04x: 0x%04x\n", portnum, value);

    switch (portnum) {

        case 0x3c4: // CGA, EGA, VGA sequencer index TODO
            active_ega_sequencer_reg = value & 0xFF;
            if (active_ega_sequencer_reg == 2) {
                ega_sequencer_bitplanemask = (value>>8) & 0xFF;
            }
            break;

        case 0x3ce: { // VGA graphics index
            // reg 0x0: set/reset          default 0x00
            // reg 0x1: enable set/reset   default 0x00
            // reg 0x2: color compare      default 0x00
            // reg 0x3: function select / data/rotate default 0x00
            // reg 0x4: read map select    default 0x00
            // reg 0x5: mode               default 0x00
            // reg 0x6: misscellaneous
            // reg 0x7: color don't care   default 0xF0
            // reg 0x8: bit mask           default 0xFF
            active_ega_reg = value & 0xFF; // reg 0x3ce
            uint8_t v = value >> 8; // reg 0x3cf
            if (active_ega_reg == 0x8) ega_bitmask = v;
            if (active_ega_reg == 0x5) ega_mode = v;
            if (active_ega_reg == 0x3) ega_function = v;
            if (active_ega_reg == 0x0) ega_setreset = v;
            if (active_ega_reg == 0x1) ega_enable_setreset = v;
            if (active_ega_reg == 0x2) ega_color_compare = v;
            if (active_ega_reg == 0x7) ega_color_dont_care = v;
            if (active_ega_reg == 0x4) ega_read_map_select = v;
            if (active_ega_reg == 0x6) {
                printf("Miscellaneous Register\n");
                exit_or_restart(1);
            }
            break;
        }

        case 0x3d0:
        case 0x3d4: { // CRT graphics index
            active_crt_reg = value & 0xFF; // reg 0x3d4
            uint8_t v = value >> 8; // reg 0x3d5
            if (active_crt_reg == 0xc) vidmemory = v << 11;
            if (active_crt_reg != 0xc) {
                printf("crt: unknown register %i\n", active_crt_reg);
                exit_or_restart(1);
            }
            //printf("port16 0x03d4: %i\n", value);
            //exit_or_restart(1);
            break;
        }

        default:
            printf("Error Unknown out16 VGA port: 0x%04x\n", portnum);
            exit_or_restart(1);
            break;
    }
}


void VGA_out(uint16_t portnum, uint8_t value) {

    //printf("VGA out 0x%04x: %i\n", portnum, value);
    switch (portnum) {

        case 0x3b4: // index register, selects which register [0-11h] is to be accessed through port 3B5
            break;

        case 0x3b5: // data register [0-11h] selected by port 3B4, registers 0C-0F may be read.  If a read occurs without the adapter installed, FFh is returned.  (see 6845)
            break;

        case 0x3b8: // 6845 Mode control register
            monochrome_mode = value;
            if (value & 2) vidmode = 0x14;
            break;

        case 0x3c0: // EGA TODO
            break;

        case 0x3c4: // EGA TODO
            active_ega_sequencer_reg = value;
            break;

        case 0x3c5: // EGA TODO
            if (active_ega_sequencer_reg == 2) ega_sequencer_bitplanemask = value;
            break;

        case 0x3c8: // VGA video DAC PEL address
            // TODO
            break;

        case 0x3c9: // VGA video DAC
            // TODO
            break;

        case 0x3ce: // EGA
            active_ega_reg = value;
            break;

        case 0x3cf: // EGA TODO
            if (active_ega_reg == 0x8) ega_bitmask = value;
            if (active_ega_reg == 0x5) ega_mode = value;
            if (active_ega_reg == 0x3) ega_function = value;
            if (active_ega_reg == 0x0) ega_setreset = value;
            if (active_ega_reg == 0x1) ega_enable_setreset = value;
            if (active_ega_reg == 0x2) ega_color_compare = value;
            if (active_ega_reg == 0x7) ega_color_dont_care = value;
            if (active_ega_reg == 0x4) ega_read_map_select = value;
            if (active_ega_reg == 0x6) {
                printf("Miscellaneous register\n");
                exit_or_restart(1);
            }
            break;

        case 0x3d0:
        case 0x3d4: // EGA/CGA
            active_crt_reg = value;
            break;

        case 0x3d5: // EGA/CGA
            switch (active_crt_reg) {
                case 0x0:
                case 0x1:
                case 0x2:
                case 0x3:
                case 0x4:
                case 0x5:
                case 0x6:
                case 0x7:
                case 0x8:
                case 0x9:
                case 0xA:
                case 0xB:
                    break;
                case 0xc:
                    vidmemory = value << 11;
                    break;
                case 0xD:
                case 0xE:
                case 0xF:
                    break;

                default:
                    printf("ega: unknown register %i\n", active_crt_reg);
                    exit_or_restart(1);
                    break;
            }
            //printf("port 0x03d5: %i, index: %i\n", value, active_crt_reg);
            //exit_or_restart(1);
            break;

        case 0x3d8: // 6845 Mode control register (Color) http://www.minuszerodegrees.net/oa/OA%20-%20IBM%20Color%20Graphics%20Monitor%20Adapter%20(CGA).pdf
            // 0x1a = 011010b
            // bit 0: 40x25 alphanumeric
            // bit 1: 320x200 graphic mode
            // bit 2: selects color mode
            // bit 3: enables the video signal
            // bit 4: high resolution 640x200
            // bit 5: blinking
            switch(value)
            {
                case 0x0: // just disable
                    break;

                case 0x4: // ????
                    vidmode = 0x4; // 320x200 color
                    vidmemory = 0x18000;
                    break;

                case 0xe: // 001110b
                    vidmode = 0x4; // 320x200 color
                    vidmemory = 0x18000;
                    break;

                case 0x1a:
                    vidmode = 0x6; // 640x200 monochrome black / white
                    vidmemory = 0x18000;
                    break;

                case 0x1e:
                    vidmode = 0x6; // 640x200 monochrome, but color
                    vidmemory = 0x18000;
                    break;

                case 0x28: // 101000
                    vidmode = 0x1; // 320x200 text with color TODO
                    vidmemory = 0x18000;
                    break;

                case 0x2e: // 101000
                    vidmode = 0x4; // 320x200 color
                    vidmemory = 0x18000;
                    break;

                default:
                    printf("unknown CGA graphics mode 0x%02x\n", value);
                    exit_or_restart(1);
                    break;
            }
            break;

        case 0x3d9: // color select register
            // bit 0 select blue color
            // bit 1 selects green color
            // bit 2 select red color
            // bit 3 selects intensified color
            // bit 4 selects alternate intensified color
            // bit 5 selects isactive color set
            break;

        default:
            printf("Error Unknown out VGA port\n");
            exit_or_restart(1);
            break;
    }
}

// call to vesa function to change the window. Used only by this emulator
void VGA_Vesa_Window_Function() {
    int mode = regs.byteregs[regbh];
    int window_number = regs.byteregs[regbl];
    int window_position = regs.wordregs[regdx];

    printf("vesa call mode=%i window_number=%i window_position=%i\n", mode, window_number, window_position);

    switch(mode)
    {
        case 0: // select super vga video memory window
            if (window_number == 1) {
                regs.wordregs[regax] = 0x014f; // function called failed
                break;
            }
            regs.wordregs[regax] = 0x004f;
            break;

        default:
            printf("Error: Unknown VESA call mode\n");
            PrintStatus();
            exit_or_restart(1);
            break;
    }



    //exit_or_restart(1);
}


uint32_t SetBitplanes(uint32_t i) {
    return
            ((i & 1) ? 0x000000ff : 0) |
            ((i & 2) ? 0x0000ff00 : 0) |
            ((i & 4) ? 0x00ff0000 : 0) |
            ((i & 8) ? 0xff000000 : 0);
}

void VGA_write(uint32_t addr32, uint8_t value) {
    //printf("VGA_write 0x%08x=0x%02x ega_setreset=0x%02x\n", addr32, value, ega_sequencer_bitplanemask);
    if ((vidmode == 0x0d) || (vidmode == 0x10)) {
        // write mode 0 read-modify-write
        // function 0 (replace)
        // enable_setreset = 15
        //printf("ega setreset %i\n", ega_setreset);
        //if ((ega_mode != 8) && (ega_mode != 0)) {printf("ega mode %i\n", ega_mode); exit_or_restart(1);}
        //if (ega_bitmask != 255) printf("ega mask %i\n", ega_bitmask);
        //if (ega_function != 0) printf("ega function %i\n", ega_function);
        //if (ega_enable_setreset != 15) printf("ega enable_setreset %i\n", ega_enable_setreset);
        //uint8_t mask = ega_setreset & ega_bitmask;
        //value &=
        // expand to the 4 bit planes
        //ega_setreset = 0xFF;
        for (int i = 0; i < 8; i++) {
            if ((ega_bitmask & (1 << (7 - i))) == 0) continue;
            uint8_t c = vgaram[(addr32 << 3) + i]&0xF;

            // TODO definitely wrong
            // this works for scr
            /*
            if ((value & (1 << (7 - i))) != 0) {
                vgaram[(addr32 << 3) + i] = c | (ega_sequencer_bitplanemask);
            } else {
                vgaram[(addr32 << 3) + i] = c & (~ega_sequencer_bitplanemask);
            }
            */
            // this works for fs4
            vgaram[(addr32 << 3) + i] = (c & ~ega_enable_setreset) | ega_setreset;

            //ega_setreset
            //ega_enable_setreset
            //ega_bitmask
        }

/*
            uint32_t c = (value << 24) | (value << 16) | (value << 8) | (value);
            uint32_t full_enable_set_reset = SetBitplanes(ega_enable_setreset & 0xF);
            uint32_t full_not_enable_set_reset = ~full_enable_set_reset;
            uint32_t full_enable_and_set_reset = SetBitplanes(ega_setreset & 0xF) & full_enable_set_reset;
            c = (c & full_not_enable_set_reset) | full_enable_and_set_reset;
            uint32_t full_bit_mask = (ega_bitmask << 24) | (ega_bitmask << 16) | (ega_bitmask << 8) | (ega_bitmask);

            uint32_t c2 = 0;
            for (int i = 0; i < 8; i++) {
                uint32_t col = vgaram[(addr32 << 3) + i];

                c2 |= ((col >> 3) & 1) << (31 - i);
                c2 |= ((col >> 2) & 1) << (23 - i);
                c2 |= ((col >> 1) & 1) << (15 - i);
                c2 |= ((col >> 0) & 1) << (7  - i);

            }
            c = (c & full_bit_mask) | (c2 & ~full_bit_mask);

            for (int i = 0; i < 8; i++) {
                vgaram[(addr32 << 3) + i] =
                        (((c >> (31-i)) & 1) << 3) |
                        (((c >> (23-i)) & 1) << 2) |
                        (((c >> (15-i)) & 1) << 1) |
                        (((c >>  (7-i)) & 1) << 0);

            }
*/
        /*
        for (int i = 0; i < 8; i++) {
            uint32_t c = value & (1 << (7 - i));
            // mode replace
            if (c == 0) {
                //vgaram[(addr32 << 3) + i] &= ~ega_bitmask;
                if (ega_bitmask&1) vgaram[(addr32 << 3) + i] &= ~0x1;
                if (ega_bitmask&2) vgaram[(addr32 << 3) + i] &= ~0x2;
                if (ega_bitmask&4) vgaram[(addr32 << 3) + i] &= ~0x4;
                if (ega_bitmask&8) vgaram[(addr32 << 3) + i] &= ~0x8;
            } else {
                //vgaram[(addr32 << 3) + i] |= ega_bitmask;
                if (ega_bitmask&1) vgaram[(addr32 << 3) + i] |= 0x1;
                if (ega_bitmask&2) vgaram[(addr32 << 3) + i] |= 0x2;
                if (ega_bitmask&4) vgaram[(addr32 << 3) + i] |= 0x4;
                if (ega_bitmask&8) vgaram[(addr32 << 3) + i] |= 0x8;
            }
        }
         */
    } else {
        vgaram[addr32] = value;
    }
}

uint8_t VGA_read(uint32_t addr32) {
    // printf("VGA_read\n");
    if ((vidmode == 0x0d) || (vidmode == 0x10)) {

        if (ega_mode & 8) {
            // read mode 1, return by color compare
            if ((ega_color_dont_care != 15) && (ega_color_dont_care != 0)) printf("dont %i\n", ega_color_dont_care);
            if (ega_color_dont_care == 0)
                return 0xFF; // TODO, this is too easy, but good enough for FS4. See pcintern.pdf
            uint8_t iscolor = 0;
            for (int i = 0; i < 8; i++) {
                uint8_t c = vgaram[(addr32 << 3) + i];
                if (c == ega_color_compare) iscolor |= (1 << (7 - i));
            }
            return iscolor;
        } else {
            // read mode 0, return the bitplane
            uint8_t iscolor = 0;
            for (int i = 0; i < 8; i++) {
                uint8_t c = vgaram[(addr32 << 3) + i];
                if (c & (1 << ega_read_map_select)) iscolor |= (1 << (7 - i));
            }
            return iscolor;
        }
    }
    return vgaram[addr32];
}

void VGA_Draw() {

    switch (vidmode) {
        case 0x0: // TODO, this is wrong use 8x8 font, see http://www.cns.utoronto.ca/~pkern/stuff/cterm+/cterm/int-10
        case 0x2:
            for (int jj = 0; jj < 25; jj++)
                for (int ii = 0; ii < 80; ii++) {
                    int x = vgaram[vidmemory + jj * 160 + ii * 2];
                    for (int j = 0; j < 16; j++)
                        for (int i = 0; i < 8; i++)
                            pixels[SCREEN_WIDTH * (j + jj * 16) + (i + ii * 8)] =
                                    0xFF000000 | (int10_font_16[(x << 4) | j] & (1 << (7 - i)) ? -1 : 0);
                }
            break;

        case 0x1: // 320x200 char mode color cga
            for (int jj = 0; jj < 25; jj++)
                for (int ii = 0; ii < 40; ii++) {
                    int x = vgaram[vidmemory + jj * 80 + ii * 2];
                    for (int j = 0; j < 16; j++)
                        for (int i = 0; i < 8; i++)
                            pixels[SCREEN_WIDTH * (j + jj * 16) + (i + ii * 8)] =
                                    0xFF000000 | (int10_font_16[(x << 4) | j] & (1 << (7 - i)) ? -1 : 0);
                }
            break;

        case 0x3:
            for (int jj = 0; jj < 25; jj++)
                for (int ii = 0; ii < 80; ii++) {
                    int x = vgaram[vidmemory + jj * 160 + ii * 2];
                    for (int j = 0; j < 16; j++)
                        for (int i = 0; i < 8; i++)
                            pixels[SCREEN_WIDTH * (j + jj * 16) + (i + ii * 8)] =
                                    0xFF000000 | (int10_font_16[(x << 4) | j] & (1 << (7 - i)) ? -1 : 0);
                }
            break;

        case 0x4: // CGA 320x200 4 Colors
        case 0x5: // CGA 320x200 4 Colors
        {
            int pal[4] = {0xFF000000, 0xFFAAAA00, 0xFFAA00AA, 0xFFAAAAAA};
            for (uint32_t j = 0; j < 200; j++)
                for (uint32_t i = 0; i < 80; i++) {
                    uint8_t c = vgaram[/*vidmemory*/0x18000 + ((j & 1) << 13) + ((j >> 1) * 80) + i];
                    for (uint32_t ii = 0; ii < 4; ii++) {
                        int color = (c >> (6 - (ii * 2))) & 3;
                        pixels[SCREEN_WIDTH * j + i * 4 + ii] = pal[color];
                    }
                }
            break;
        }

        case 0x6: // CGA 640x200 2 Colors
        {
            int pal[2] = {0xFF000000, 0xFFAAAAAA};
            for (uint32_t j = 0; j < 200; j++)
                for (uint32_t i = 0; i < 80; i++) {
                    uint8_t c = vgaram[/*vidmemory*/0x18000 + ((j & 1) << 13) + ((j >> 1) * 80) + i];
                    for (uint32_t ii = 0; ii < 8; ii++) {
                        int color = (c >> (7 - ii)) & 1;
                        pixels[SCREEN_WIDTH * j + i * 8 + ii] = pal[color];
                    }
                }
            break;
        }

        case 0x7:
            /*
            07h = 80x25	9x14	     mono    1	B000 MDA/Hercules
                = 80x25			     8	     EGA/VGA
                  = 80x25	9x14	     mono	B000 ATI VIP
            */
            for (int jj = 0; jj < 25; jj++)
                for (int ii = 0; ii < 80; ii++) {
                    int x = vgaram[vidmemory + jj * 160 + ii * 2];
                    for (int j = 0; j < 14; j++)
                        for (int i = 0; i < 8; i++)
                            pixels[SCREEN_WIDTH * (j + jj * 16) + (i + ii * 8)] =
                                    0xFF000000 | (int10_font_14[(x * 14) + j] & (1 << (7 - i)) ? -1 : 0);
                }
            break;

        case 0x0d: // ega 320x200 16-color
            //printf("vidmemory 0x%06x\n", vidmemory);
            //vidmemory=0x0;
            for (uint32_t j = 0; j < 200; j++)
                for (uint32_t i = 0; i < 320; i++) {
                    uint32_t c = vgaram[vidmemory + j * 320 + i]&0xF;
                    pixels[SCREEN_WIDTH * j + i] = palette[c];
                }
            break;

        case 0x10: // ega 640x350 16-color
            for (uint32_t j = 0; j < 350; j++)
                for (uint32_t i = 0; i < 640; i++) {
                    uint32_t c = vgaram[vidmemory + j * 640 + i];
                    pixels[SCREEN_WIDTH * j + i] = palette[c];
                }
            break;

        case 0x13: // vga 320x200 256 colors
            for (int j = 0; j < 200; j++)
                for (int i = 0; i < 320; i++) {
                    uint32_t c = vgaram[vidmemory + j * 320 + i];
                    pixels[SCREEN_WIDTH * j + i] = palette[c];
                }
            break;

        case 0x14: // hercules monochrome graphics mode
            vidmemory = 0xB0000 + 0x8000 * (monochrome_mode >> 7);
            // max resolution of hercules monochrome is 720x348
            for (uint32_t j = 0; j < 348; j++)
                for (uint32_t i = 0; i < 90; i++) {
                    uint8_t c = vgaram[vidmemory + ((j & 3) << 13) + ((j >> 2) * 90) + i];
                    for (uint32_t ii = 0; ii < 8; ii++) {
                        pixels[SCREEN_WIDTH * j + i * 8 + ii] = c & (1 << (7 - ii)) ? -1 : 0;
                    }
                }
            break;

        case 0x100: // vga 64ßx400 256 colors
            for (int j = 0; j < 400; j++)
                for (int i = 0; i < 640; i++) {
                    uint32_t c = vgaram[j * 640 + i];
                    pixels[SCREEN_WIDTH * j + i] = palette[c];
                }
            break;

        default:
            printf("unknown video mode %i\n", vidmode);
            exit_or_restart(1);
            break;
    }
}

void VGA_Init() {
    vidmode = 3;
    vidmemory = 0x18000;

    monochrome_mode = 0;
    active_ega_reg = 0;
    ega_bitmask = 0xFF;
    ega_mode = 0x0;
    ega_function = 0x0;
    ega_enable_setreset = 0x0;
    ega_setreset = 0xFF;
    ega_color_compare = 0x0;
    ega_color_dont_care = 0x0;
    ega_read_map_select = 0x0;
    active_ega_sequencer_reg = 0x0;
    ega_sequencer_bitplanemask = 0x0F;

    active_crt_reg = 0;

    cursor_column = 0;
    cursor_row = 0;
    if (vgaram == 0) {
        vgaram = malloc(0x10001 * 2 * 8);
    }
    memset(vgaram, 0, 0x10001 * 2 * 8);
    memset(palette, 0, 256 * 4);

    for (int i = 0; i < 248; i++) {
        uint32_t r = vga_palette[i][0];
        uint32_t g = vga_palette[i][1];
        uint32_t b = vga_palette[i][2];
        palette[i] = 0xFF000000 | (b << 18) | (g << 10) | (r << 2);
    }

}
