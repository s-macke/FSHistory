/****************************************************************************
*                                                                           *
*                            Third Year Project                             *
*                                                                           *
*                            An IBM PC Emulator                             *
*                          For Unix and X Windows                           *
*                                                                           *
*                             By David Hedley                               *
*                                                                           *
*                                                                           *
* This program is Copyrighted.  Consult the file COPYRIGHT for more details *
*                                                                           *
****************************************************************************/

#include "global.h"

#include "../wasm_libc_wrapper/stddef.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/string.h"
#include "../wasm_libc_wrapper/ctype.h"

#include "debugger.h"
#include "cpu.h"
#include "disasm.h"

static int running;
static int breakpoint;
static int debug_abort;
static BYTE *bpoint;

static int numbase = 16;

#define mylower(c) ((c >= 'A' && c <= 'Z') ? c-'A'+'a' : c)

static BYTE instruction_byte;

static char wordp[] = "word ptr ";
static char bytep[] = "byte ptr ";
static char blank[] = "";

static char *get_byte_reg(unsigned ModRM)
{
    return byte_reg[(ModRM & 0x38) >> 3];
}

static char *get_word_reg(unsigned ModRM)
{
    return word_reg[(ModRM & 0x38) >> 3];
}

static char *get_seg_reg(unsigned ModRM)
{
    return seg_reg[(ModRM & 0x38) >> 3];
}

static unsigned get_d8(BYTE *seg, unsigned *off)
{
    return GetMemInc(seg, (*off));
}

static unsigned get_d16(BYTE *seg, unsigned *off)
{
    unsigned num = GetMemInc(seg, (*off));
    num += GetMemInc(seg, (*off)) << 8;
    return num;
}

static char *get_mem(unsigned ModRM, BYTE *seg, unsigned *off, char **reg, char *msg)
{
    static char buffer[100];
    int num;
    char ch;

    switch(ModRM & 0xc0)
    {
    case 0x00:
        if ((ModRM & 0x07) != 6)
            sprintf(buffer,"%s[%s]", msg, index_reg[ModRM & 0x07]);
        else
            sprintf(buffer,"%s[%04X]", msg, get_d16(seg, off));
        break;
    case 0x40:
        if ((num = (INT8)get_d8(seg, off)) < 0)
        {
            ch = '-';
            num = -num;
        }
        else
            ch = '+';
        sprintf(buffer,"%s[%s%c%02X]", msg, index_reg[ModRM & 0x07], ch, num);
        break;
    case 0x80:
        if ((num = (INT16)get_d16(seg, off)) < 0)
        {
            ch = '-';
            num = -num;
        }
        else
            ch = '+';
        sprintf(buffer,"%s[%s%c%04X]", msg, index_reg[ModRM & 0x07], ch, num);
        break;
    case 0xc0:
        strcpy(buffer, reg[ModRM & 0x07]);
        break;
    }

    return buffer;
}

static WORD get_disp(BYTE *seg, unsigned *off)
{
    unsigned disp = GetMemInc(seg, (*off));

    return (WORD)(*off + (INT32)((INT8)disp));
}

static WORD get_disp16(BYTE *seg, unsigned *off)
{
    unsigned disp = GetMemInc(seg, (*off));
    disp += GetMemInc(seg, (*off)) << 8;

    return (WORD)(*off + (INT32)((INT16)disp));
}

static void print_instruction(BYTE *seg, unsigned off, int *tab, char *buf)
{
    unsigned ModRM = GetMemB(seg,off);
    sprintf(buf, "%-6s ", itext[tab[(ModRM & 0x38) >> 3]]);
}
    
static unsigned decode_br8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf, "%s,%s", get_mem(ModRM, seg, &off, byte_reg, blank), get_byte_reg(ModRM));
    return off;
}

static unsigned decode_r8b(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf,"%s,%s", get_byte_reg(ModRM), get_mem(ModRM, seg, &off, byte_reg, blank));
    return off;
}

static unsigned decode_wr16(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf, "%s,%s", get_mem(ModRM, seg, &off, word_reg, blank), get_word_reg(ModRM));
    return off;
}

static unsigned decode_r16w(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf,"%s,%s", get_word_reg(ModRM), get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_ald8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"al,%02X",get_d8(seg, &off));
    return off;
}

static unsigned decode_axd16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"ax,%04X",get_d16(seg, &off));
    return off;
}

static unsigned decode_pushpopseg(BYTE *seg, unsigned off, char *buf)
{
    strcpy(buf, get_seg_reg(instruction_byte));
    return off;
}

static unsigned decode_databyte(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%02X", instruction_byte);
    return off;
}

static unsigned decode_wordreg(BYTE *seg, unsigned off, char *buf)
{
    strcat(buf, word_reg[instruction_byte & 0x7]);
    return off;
}


static unsigned decode_cond_jump(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%-5s %04X", condition[instruction_byte & 0xf], get_disp(seg, &off));
    return off;
}

static unsigned decode_bd8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, byte_reg, bytep);
    sprintf(buf,"%s,%02X", mem, get_d8(seg, &off));
    return off;
}

static unsigned decode_wd16(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, word_reg, wordp);
    sprintf(buf,"%s,%04X", mem, get_d16(seg, &off));
    return off;
}

static unsigned decode_wd8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, word_reg, wordp);
    sprintf(buf,"%s,%02X", mem, get_d8(seg, &off));
    return off;
}

static unsigned decode_ws(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,%s", get_mem(ModRM, seg, &off, word_reg, blank), get_seg_reg(ModRM));
    return off;
}

static unsigned decode_sw(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,%s", get_seg_reg(ModRM), get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_w(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    strcpy(buf, get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_b(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    strcpy(buf, get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_xchgax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "ax,%s", word_reg[instruction_byte & 0x7]);
    return off;
}

static unsigned decode_far(BYTE *seg, unsigned off, char *buf)
{
    unsigned offset = get_d16(seg, &off);

    sprintf(buf,"%04X:%04X", get_d16(seg, &off), offset);
    return off;
}

static unsigned decode_almem(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"al,[%04X]", get_d16(seg, &off));
    return off;
}

static unsigned decode_axmem(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"ax,[%04X]", get_d16(seg, &off));
    return off;
}

static unsigned decode_memal(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"[%04X],al", get_d16(seg, &off));
    return off;
}

static unsigned decode_memax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"[%04X],ax", get_d16(seg, &off));
    return off;
}

static unsigned decode_string(BYTE *seg, unsigned off, char *buf)
{
    if (instruction_byte & 0x01)
        strcat(buf,"w");
    else
        strcat(buf,"b");

    return off;
}

static unsigned decode_rd(BYTE *seg, unsigned off, char *buf)
{
    if ((instruction_byte & 0xf) > 7)
        sprintf(buf,"%s,%04X", word_reg[instruction_byte & 0x7], get_d16(seg, &off));
    else
        sprintf(buf,"%s,%02X", byte_reg[instruction_byte & 0x7], get_d8(seg, &off));

    return off;
}

static unsigned decode_d16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%04X", get_d16(seg, &off));
    return off;
}

static unsigned decode_int3(BYTE *seg, unsigned off, char *buf)
{
    strcpy(buf, "3");
    return off;
}

static unsigned decode_d8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%02X", get_d8(seg, &off));
    return off;
}

static unsigned decode_bbit1(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,1", get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_wbit1(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,1", get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_bbitcl(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,cl", get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_wbitcl(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,cl", get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_disp(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%04X", get_disp(seg, &off));
    return off;
}

static unsigned decode_escape(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM  = GetMemInc(seg, off);
    sprintf(buf,"%d,%s", instruction_byte & 0x7,
            get_mem(ModRM, seg, &off, nul_reg, blank));
    return off;
}

static unsigned decode_adjust(BYTE *seg, unsigned off, char *buf)
{
    unsigned num = GetMemInc(seg, off);

    if (num != 10)
        sprintf(buf, "%02X", num);
    return off;
}

static unsigned decode_d8al(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%02X,al", get_d8(seg, &off));
    return off;
}

static unsigned decode_d8ax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%02X,ax", get_d8(seg, &off));
    return off;
}

static unsigned decode_axd8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "ax,%02X", get_d8(seg, &off));
    return off;
}

static unsigned decode_far_ind(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf, "far %s", get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_portdx(BYTE *seg, unsigned off, char *buf)
{
    switch (instruction_byte)
    {
    case 0xec:
        strcpy(buf,"al,dx"); break;
    case 0xed:
        strcpy(buf,"ax,dx"); break;
    case 0xee:
        strcpy(buf,"dx,al"); break;
    case 0xef:
        strcpy(buf,"dx,ax"); break;
    }
     
    return off;
}

static unsigned decode_disp16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%04X", get_disp16(seg, &off));
    return off;
}

static unsigned decode_f6(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemB(seg, off);
    if ((ModRM & 0x38) == 0x00)
        return decode_bd8(seg, off, buf);

    return decode_b(seg, off, buf);
}

static unsigned decode_f7(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemB(seg, off);
    if ((ModRM & 0x38) == 0x00)
        return decode_wd16(seg, off, buf);

    return decode_w(seg, off, buf);
}

static unsigned decode_ff(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = (GetMemB(seg, off) & 0x38) >> 3;

    if (ModRM == 3 || ModRM == 5)
        return decode_far_ind(seg, off, buf);

    return decode_w(seg, off, buf);
}

static unsigned decode_bioscall(BYTE *seg, unsigned off, char *buf)
{
    unsigned addr;

    if (GetMemB(seg, off) == 0xf1)
    {
        off = (WORD)(off + 1);
        addr = GetMemInc(seg, off);
        addr += GetMemInc(seg, off) << 8;
        addr += GetMemInc(seg, off) << 16;
        addr += GetMemInc(seg, off) << 24;
        sprintf(buf, "bios   %08X",addr);
    }
    else
        sprintf(buf, "db     F1");

    return off;
}

unsigned disasm(unsigned seg, unsigned off, BYTE *memory, char *buffer)
{
    BYTE *segp = &memory[(seg << 4)];
    struct Disasm *d;

    instruction_byte = GetMemInc(segp, off);
    d = &disasm_table[instruction_byte];

    if (d->supp != NULL)
        print_instruction(segp, off, d->supp, buffer);
    else
        sprintf(buffer, (d->flags & DF_NOSPACE) ? "%s" : "%-6s ",
                itext[d->text]);
//printf("instruction byte 0x%x\n", instruction_byte);
    if (d->type != NULL)
        off = (d->type)(segp, off, &buffer[strlen(buffer)]);

    return off;
}

static unsigned disassemble(unsigned seg, unsigned off, BYTE *memory, int count)
{
    char buffer1[80];
    char buffer2[80];
    char buffer3[3];
    unsigned newoff;

    for (; !debug_abort && count > 0; count--)
    {
        do
        {
            printf("%04X:%04X ", seg, off);
            buffer1[0] = '\0';
            newoff = disasm(seg, off, memory, buffer1);
            buffer2[0] = '\0';
            for (; off < newoff; off++)
            {
                sprintf(buffer3,"%02X", GetMemB(&memory[seg << 4], off));
                strcat(buffer2,buffer3);
            }
            printf("%-14s%s\n", buffer2,buffer1);
        } while (disasm_table[instruction_byte].flags & DF_PREFIX);
    }
    return off;
}

