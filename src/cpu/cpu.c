#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

#include "../devices/ports.h"
#include "../devices/ram.h"
#include "../devices/rom.h"
#include "../devices/pic.h"
#include "../devices/pit.h"
#include"../disasm/debugger.h"
#include"../devices/keyb.h"

#include "cpu.h"

// registers
uint16_t segregs[6];
uint16_t ip;
union regstruct regs;
static uint8_t cf, zf, pf, af, sf, tf, ifl, df, of, ntf, ioplf, acf;

// helper variables
static uint16_t useseg, segoverride;
static uint8_t hltstate;
static uint16_t saveip;
static uint16_t oper1, oper2, disp16;
static uint8_t oper1b, oper2b;
static uint8_t mode, reg, rm;

const static uint8_t byteregtable[8] = {regal, regcl, regdl, regbl, regah, regch, regdh, regbh};

static const uint8_t parity[0x100] = {
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

#define signext32(value)             ((int32_t)(int16_t)(value))
#define signext16(value)             ((int16_t)(int8_t)(value))
#define getreg32(regid)              regs.dwordregs[regid]
#define getreg16(regid)              regs.wordregs[(regid)<<1]
#define getreg8(regid)               regs.byteregs[byteregtable[regid]]
#define putreg32(regid, writeval)    regs.dwordregs[regid] = writeval
#define putreg16(regid, writeval)    regs.wordregs[(regid)<<1] = writeval
#define putreg8(regid, writeval)     regs.byteregs[byteregtable[regid]] = writeval
#define AdvanceIP(x)                 ip += (x)

#include "flags.c"
#include "arith.c"
#include "repops.c"
#include "helper.c"

static inline void push16(uint16_t pushval) {
    regs.wordregs[regsp] -= 2;
    Write16Long(segregs[regss], regs.wordregs[regsp], pushval);
}

static inline void push32(uint32_t pushval) {
    regs.wordregs[regsp] -= 4;
    Write32Long(segregs[regss], regs.wordregs[regsp], pushval);
}

static inline uint16_t pop16() {
    uint16_t tempval = Read16LongUnsafe(segregs[regss], regs.wordregs[regsp]);
    regs.wordregs[regsp] += 2;
    return tempval;
}

static inline uint32_t pop32() {
    uint32_t tempval = Read32LongUnsafe(segregs[regss], regs.wordregs[regsp]);
    regs.wordregs[regsp] += 4;
    return tempval;
}

void intcall86(uint8_t intnum) {
    push16(makeflags());
    push16(segregs[regcs]);
    push16(ip);
    segregs[regcs] = Read16LongUnsafe(0, (uint16_t) intnum * 4 + 2);
    ip = Read16LongUnsafe(0, (uint16_t) intnum * 4);
    ifl = 0;
    tf = 0;
}

// for reference https://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM_and_SIB_bytes
static void modregrm() {
    uint8_t addrbyte = Read8LongUnsafe(segregs[regcs], ip);
    AdvanceIP(1);
    mode = addrbyte >> 6u;
    reg = ((addrbyte >> 3u) & 7);
    rm = addrbyte & 7u; // r/m field
    switch (mode) {
        case 0:  // register indirect addressing mode
            disp16 = 0;
            if (rm == 6) {
                disp16 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
            }
/*
            if (((rm == 2) || (rm == 3)) && !segoverride) { // bp + si or bp + di
                useseg = segregs[regss];
            }
            */
            break;
        case 1: // one byte signed displacment
            disp16 = signext16(Read8LongUnsafe(segregs[regcs], ip));
            AdvanceIP(1);
/*
            if (((rm == 2) || (rm == 3) || (rm == 6)) && !segoverride) {  // // bp + si or bp + di
                useseg = segregs[regss];
            }
            */
            break;
        case 2: // two byte signed displacment
            disp16 = Read16LongUnsafe(segregs[regcs], ip);
            AdvanceIP(2);
/*
            if (((rm == 2) || (rm == 3) || (rm == 6)) && !segoverride) {  // // bp + si or bp + di or bp+diso16
                useseg = segregs[regss];
            }
*/
            break;
        default: // register address mode
            disp16 = 0;
    }
}

static inline uint8_t op_adc8() {
    uint8_t res8 = oper1b + oper2b + cf;
    flag_adc8(oper1b, oper2b, cf);
    return res8;
}

static inline uint16_t op_adc16() {
    uint16_t res16 = oper1 + oper2 + cf;
    flag_adc16(oper1, oper2, cf);
    return res16;
}

static inline uint8_t op_add8() {
    uint8_t res8 = oper1b + oper2b;
    flag_add8(oper1b, oper2b);
    return res8;
}

static inline uint16_t op_add16() {
    uint16_t res16 = oper1 + oper2;
    flag_add16(oper1, oper2);
    return res16;
}

static inline uint8_t op_and8() {
    uint8_t res8 = oper1b & oper2b;
    flag_logic8(res8);
    return res8;
}

static inline uint16_t op_and16() {
    uint16_t res16 = oper1 & oper2;
    flag_logic16(res16);
    return res16;
}

static inline uint8_t op_or8() {
    uint8_t res8 = oper1b | oper2b;
    flag_logic8(res8);
    return res8;
}

static inline uint16_t op_or16() {
    uint16_t res16 = oper1 | oper2;
    flag_logic16(res16);
    return res16;
}

static inline uint8_t op_xor8() {
    uint8_t res8 = oper1b ^oper2b;
    flag_logic8(res8);
    return res8;
}

static inline uint16_t op_xor16() {
    uint16_t res16 = oper1 ^oper2;
    flag_logic16(res16);
    return res16;
}

static inline uint8_t op_sub8() {
    uint8_t res8 = oper1b - oper2b;
    flag_sub8(oper1b, oper2b);
    return res8;
}

static inline uint16_t op_sub16() {
    uint16_t res16 = oper1 - oper2;
    flag_sub16(oper1, oper2);
    return res16;
}

static inline uint8_t op_sbb8() {
    uint8_t res8 = oper1b - (oper2b + cf);
    flag_sbb8(oper1b, oper2b, cf);
    return res8;
}

static inline uint16_t op_sbb16() {
    uint16_t res16 = oper1 - (oper2 + cf);
    flag_sbb16(oper1, oper2, cf);
    return res16;
}

static inline uint32_t getea(uint8_t rmval) {
    uint32_t tempea = 0;

    switch (mode) {
        case 0:
            switch (rmval) {
                case 0:
                    tempea = regs.wordregs[regbx] + regs.wordregs[regsi];
                    break;
                case 1:
                    tempea = regs.wordregs[regbx] + regs.wordregs[regdi];
                    break;
                case 2:
                    tempea = regs.wordregs[regbp] + regs.wordregs[regsi];
                    if (!segoverride) useseg = segregs[regss];
                    break;
                case 3:
                    tempea = regs.wordregs[regbp] + regs.wordregs[regdi];
                    if (!segoverride) useseg = segregs[regss];
                    break;
                case 4:
                    tempea = regs.wordregs[regsi];
                    break;
                case 5:
                    tempea = regs.wordregs[regdi];
                    break;
                case 6:
                    tempea = disp16;
                    break;
                case 7:
                    tempea = regs.wordregs[regbx];
                    break;
            }
            break;

        case 1:
        case 2:
            switch (rmval) {
                case 0:
                    tempea = regs.wordregs[regbx] + regs.wordregs[regsi] + disp16;
                    break;
                case 1:
                    tempea = regs.wordregs[regbx] + regs.wordregs[regdi] + disp16;
                    break;
                case 2:
                    tempea = regs.wordregs[regbp] + regs.wordregs[regsi] + disp16;
                    if (!segoverride) useseg = segregs[regss];
                    break;
                case 3:
                    tempea = regs.wordregs[regbp] + regs.wordregs[regdi] + disp16;
                    if (!segoverride) useseg = segregs[regss];
                    break;
                case 4:
                    tempea = regs.wordregs[regsi] + disp16;
                    break;
                case 5:
                    tempea = regs.wordregs[regdi] + disp16;
                    break;
                case 6:
                    tempea = regs.wordregs[regbp] + disp16;
                    if (!segoverride) useseg = segregs[regss];
                    break;
                case 7:
                    tempea = regs.wordregs[regbx] + disp16;
                    break;
            }
            break;
        default:
            printf("Error: Unknown mode for getea\n");
            exit_or_restart(1);
            break;
    }
/*
    if ((tempea&0xFFFFu) == 0xFFFF) {
        printf("Warning: critical memory access\n");
    }
*/
    uint32_t ea = (tempea & 0xFFFFu) + segbase(useseg);
    return ea;
}

void reset86() {
    hltstate = 0;

    regs.dwordregs[regeax] = 0x0;
    regs.dwordregs[regebx] = 0x0;
    regs.dwordregs[regecx] = 0x0;
    regs.dwordregs[regedx] = 0x0;
    regs.dwordregs[regebp] = 0x0;
    regs.dwordregs[regesp] = 0x0;
    regs.dwordregs[regesi] = 0x0;
    regs.dwordregs[regedi] = 0x0;
    segregs[reges] = 0x0;
    segregs[regds] = 0x0;
    segregs[regss] = 0x0;
    segregs[regcs] = 0xFFFF;
    ip = 0x0000;
    ntf = 1;
    ioplf = 3;
    cf = 0;
    of = 0;

    // dos related
    ifl = 1; // interrupts enabled because we already have a booted DOS
}

static inline uint32_t readrm32(uint8_t rmval) {
    if (mode < 3) {
        return Read32(getea(rmval));
    } else {
        return getreg32(rmval);
    }
}


static inline uint16_t readrm16(uint8_t rmval) {
    if (mode < 3) {
        return Read16(getea(rmval));
    } else {
        return getreg16(rmval);
    }
}

static inline uint8_t readrm8(uint8_t rmval) {
    if (mode < 3) {
        return Read8(getea(rmval));
    } else {
        return getreg8(rmval);
    }
}

static inline void writerm32(uint8_t rmval, uint32_t value) {
    if (mode < 3) {
        Write32(getea(rmval), value);
    } else {
        putreg32(rmval, value);
    }
}

static inline void writerm16(uint8_t rmval, uint16_t value) {
    if (mode < 3) {
        Write16(getea(rmval), value);
    } else {
        putreg16(rmval, value);
    }
}

static inline void writerm8(uint8_t rmval, uint8_t value) {
    if (mode < 3) {
        Write8(getea(rmval), value);
    } else {
        putreg8(rmval, value);
    }
}

static inline uint8_t op_grp2_8(uint8_t cnt) {
    uint16_t s;
    uint16_t shift;
    uint16_t oldcf;

    s = oper1b;
    oldcf = cf;
    if (cnt == 0) return s; // TODO, is this valid for all instructions
    cnt &= 0x1F;
    switch (reg) {
        case 0: /* ROL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x80u) ? 1 : 0;
                s = s << 1u;
                s = s | cf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 7u) & 1);
                //of = ((s & 0x80u) && cf) ? 1 : 0;
            };
            break;

        case 1: /* ROR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1u;
                s = (s >> 1u) | (cf << 7u);
            }

            if (cnt == 1) {
                of = (s >> 7u) ^ ((s >> 6u) & 1u);
            }
            break;

        case 2: /* RCL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = (s & 0x80u) ? 1 : 0;
                s = s << 1u;
                s = s | oldcf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 7u) & 1);
            }
            break;

        case 3: /* RCR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = s & 1u;
                s = (s >> 1u) | (oldcf << 7u);
            }

            if (cnt == 1) {
                of = ((s >> 7) & 1) ^ ((s >> 6) & 1);
            }
            break;

        case 4: // SHL r/m8
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x80) ? 1 : 0;
                s = (s << 1) & 0xFF;
            }
            if (cnt == 1) {
                of = (cf == ((s >> 7) & 1)) ? 0 : 1;
            }
            flag_szp8((uint8_t) s);
            break;

        case 5: /* SHR r/m8 */
            if (cnt == 1) {
                of = (s & 0x80) ? 1 : 0;
            }
            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1;
                s = s >> 1;
            }

            flag_szp8((uint8_t) s);
            break;

        case 7: /* SAR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                uint16_t msb = s & 0x80u;
                cf = s & 1u;
                s = (s >> 1u) | msb;
            }
            if (cnt == 1) {
                of = 0;
            }
            flag_szp8((uint8_t) s);
            break;
        default:
            printf("Error: unknown grp2_8 op\n");
            exit_or_restart(1);
            break;
    }
    return s & 0xFF;
}

static inline uint16_t op_grp2_16(uint8_t cnt) {

    uint32_t s;
    uint32_t shift;
    uint32_t oldcf;

    s = oper1;
    oldcf = cf;
    if (cnt == 0) return s; // TODO, is this valid for all instructions

    cnt &= 0x1F;
    switch (reg) {
        case 0: /* ROL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x8000u) ? 1 : 0;
                s = s << 1u;
                s = s | cf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 15u) & 1u);
            }
            break;

        case 1: /* ROR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1u;
                s = (s >> 1u) | ((uint16_t) cf << 15u);
            }

            if (cnt == 1) {
                of = (s >> 15u) ^ ((s >> 14u) & 1u);
            }
            break;

        case 2: /* RCL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = (s & 0x8000u) ? 1 : 0;

                s = s << 1u;
                s = s | oldcf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 15u) & 1u);
            }
            break;

        case 3: /* RCR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = s & 1u;
                s = (s >> 1u) | (oldcf << 15u);
            }

            if (cnt == 1) {
                of = (s >> 15u) ^ ((s >> 14u) & 1u);
            }
            break;

        case 4:  // SHL r/m8
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x8000u) ? 1 : 0;
                s = (s << 1u) & 0xFFFFu;
            }
            if (cnt == 1) {
                of = (cf == ((s >> 15u) & 1)) ? 0 : 1;
            }
            flag_szp16((uint16_t) s);
            break;

        case 5: /* SHR r/m8 */
            if (cnt == 1) {
                of = (s & 0x8000u) ? 1 : 0;
            }

            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1;
                s = s >> 1;
            }

            flag_szp16((uint16_t) s);
            break;

        case 7: /* SAR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                uint32_t msb = s & 0x8000;
                cf = s & 1;
                s = (s >> 1) | msb;
            }
            if (cnt == 1) {
                of = 0;
            }
            flag_szp16((uint16_t) s);
            break;

        default:
            printf("Error: unknown grp2_16 op\n");
            exit_or_restart(1);
            break;

    }
    return (uint16_t) s & 0xFFFF;
}

static inline void op_grp3_8() {
    uint32_t temp1, temp2;
    uint8_t res8;

    modregrm();
    oper1b = readrm8(rm);

    switch (reg) {
        case 0: /* TEST */
            flag_logic8(oper1b & Read8LongUnsafe(segregs[regcs], ip));
            AdvanceIP(1);
            break;

        case 2: /* NOT */
            res8 = ~oper1b;
            writerm8(rm, res8);
            break;

        case 3: /* NEG */
            res8 = -oper1b;
            flag_sub8(0, oper1b);
            cf = res8 ? 1 : 0;
            writerm8(rm, res8);
            break;

        case 4: /* MUL */
            //printf("mul8->16 %i %i \n", oper1b, regs.byteregs[regal]);
            op_mul8(regs.byteregs[regal], oper1b);
            break;

        case 5: /* IMUL */
            op_imul8(regs.byteregs[regal], oper1b);
            break;

        case 6: /* DIV */
            op_div8(regs.wordregs[regax], oper1b);
            break;

        case 7: /* IDIV */
            op_idiv8(regs.wordregs[regax], oper1b);
            break;

        default:
            printf("Error: Unknown grp3_8 opcode\n");
            exit_or_restart(1);
            break;
    }
}


static inline void op_grp3_16() {
    uint32_t temp1, temp2;
    uint16_t res16;

    modregrm();
    oper1 = readrm16(rm);

    switch (reg) {
        case 0: // TEST
            flag_logic16(oper1 & Read16LongUnsafe(segregs[regcs], ip));
            AdvanceIP (2);
            break;

        case 2: // NOT
            res16 = ~oper1;
            writerm16(rm, res16);
            break;

        case 3: // NEG
            res16 = -oper1;
            flag_sub16(0, oper1);
            cf = res16 ? 1 : 0;
            writerm16(rm, res16);
            break;

        case 4: /* MUL */
            op_mul16(regs.wordregs[regax], oper1);
            break;

        case 5: /* IMUL */
            op_imul16(regs.wordregs[regax], oper1);
            break;

        case 6: /* DIV */
            op_div16(((uint32_t) regs.wordregs[regdx] << 16) + regs.wordregs[regax], oper1);
            break;

        case 7: /* IDIV */
            op_idiv16(((uint32_t) regs.wordregs[regdx] << 16) + regs.wordregs[regax], oper1);
            break;

        default:
            printf("Error: Unknown grp3_16 opcode\n");
            exit_or_restart(1);
            break;

    }
}

static inline void op_grp5() {
    uint8_t tempcf;
    uint32_t ea;
    uint16_t res16;

    modregrm();
    oper1 = readrm16(rm);

    switch (reg) {
        case 0: /* INC Ev */
            oper2 = 1;
            tempcf = cf;
            res16 = op_add16();
            cf = tempcf;
            writerm16(rm, res16);
            break;

        case 1: /* DEC Ev */
            oper2 = 1;
            tempcf = cf;
            res16 = op_sub16();
            cf = tempcf;
            writerm16(rm, res16);
            break;

        case 2: /* CALL Ev */
            push16(ip);
            ip = oper1;
            break;

        case 3: /* CALL Mp */
            push16(segregs[regcs]);
            push16(ip);
            ea = getea(rm);
            ip = Read16Unsafe(ea);
            segregs[regcs] = Read16Unsafe(ea + 2);
            break;

        case 4: /* JMP Ev */
            ip = oper1;
            break;

        case 5: /* JMP Mp */
            ea = getea(rm);
            ip = Read16Unsafe(ea);
            segregs[regcs] = Read16Unsafe(ea + 2);
            break;

        case 6: /* PUSH Ev */
            push16(oper1);
            break;

        default:
            printf("Error: unknown grp5 opcode\n");
            exit_or_restart(1);
            break;
    }
}

#include "opcode66.c"

static uint32_t count = 0;

void exec86(uint32_t execloops) {

    uint8_t oldcf;
    uint16_t res16;
    uint8_t res8;
    uint8_t opcode;
    //static uint16_t trap_toggle = 0;

    while (execloops-- > 0) {

        // advance timer
        if ((count & 0xFFu) == 0) {
            PIT_count(0x100);
        }
        count++;
/*
        if (trap_toggle) {
            intcall86 (1);
        }
        trap_toggle = tf?1:0;
*/
        if (ifl && PIC_triggered) {
            hltstate = 0;
            intcall86(PIC_nextinterrupt());
        }

        if (hltstate) {
            continue;
        }

        segoverride = 0;
        useseg = segregs[regds];
        saveip = ip;

        restart: // prefix logic

        opcode = Read8LongUnsafe(segregs[regcs], ip);
        AdvanceIP(1);
        //printf("opcode: 0x%02x\n", opcode);

// 00 ADD 6
// 08 OR 6
// 10 ADC
// 18 SBB
// 20 AND
// 28 SUB
// 30 XOR
// 38 CMP
// 88 MOV

        switch (opcode) {
            case 0x0:    /* 00 ADD Eb Gb */
                modregrm();

                if ((mode == 0) && (reg == 0) && (rm == 0)) {
                    exit(1);
                }

                oper1b = readrm8(rm);
                oper2b = getreg8(reg);
                writerm8(rm, op_add8());
                break;

            case 0x1:    /* 01 ADD Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                writerm16(rm, op_add16());
                break;

            case 0x2:    /* 02 ADD Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                putreg8(reg, op_add8());
                break;

            case 0x3:    /* 03 ADD Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                putreg16(reg, op_add16());
                break;

            case 0x4:    /* 04 ADD regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                regs.byteregs[regal] = op_add8();
                break;

            case 0x5:    /* 05 ADD eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
                regs.wordregs[regax] = op_add16();
                break;

            case 0x6:    /* 06 PUSH segregs[reges] */
                push16(segregs[reges]);
                break;

            case 0x7:    /* 07 POP segregs[reges] */
                segregs[reges] = pop16();
                break;

            case 0x8:    /* 08 OR Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8(reg);
                writerm8(rm, op_or8());
                break;

            case 0x9:    /* 09 OR Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                writerm16(rm, op_or16());
                break;

            case 0xA:    /* 0A OR Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                putreg8 (reg, op_or8());
                break;

            case 0xB:    /* 0B OR Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                putreg16 (reg, op_or16());
/*
                if ((oper1 == 0xF802) && (oper2 == 0xF802)) {
                    sf = 0;    // cheap hack to make Wolf 3D think we're a 286 so it plays
                }
*/
                break;

            case 0xC:    /* 0C OR al Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = op_or8();
                break;

            case 0xD:    /* 0D OR eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                regs.wordregs[regax] = op_or16();
                break;

            case 0xE:    /* 0E PUSH segregs[regcs] */
                push16(segregs[regcs]);
                break;

            case 0xF: // 0F
                opcode = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                switch(opcode) {
                    case 0x84: // jz 16 bit
                        if (zf) {
                            ip += signext32(Read16LongUnsafe(segregs[regcs], ip));
                        }
                        AdvanceIP (2);
                        break;

                    case 0xa0: // push fs
                        push16(segregs[regfs]);
                        break;
                    case 0xa1: // pop fs
                        segregs[regfs] = pop16();
                        break;
                    case 0xa8: // push gs
                        push16(segregs[reggs]);
                        break;
                    case 0xa9: // pop gs
                        segregs[reggs] = pop16();
                        break;

                    case 0xa2: // cpuid
                        printf("cpuid 0x%08x\n", regs.dwordregs[regeax]);
                        if (regs.dwordregs[regeax] == 0) {
                            regs.dwordregs[regeax] = 0x00000001;
                            regs.dwordregs[regebx] = 0x756e6547;
                            regs.dwordregs[regecx] = 0x6c65746e;
                            regs.dwordregs[regedx] = 0x49656e69;
                        } else {
                            regs.dwordregs[regeax] = 0x00000402;
                            regs.dwordregs[regebx] = 0x00000000;
                            regs.dwordregs[regecx] = 0x00000000;
                            regs.dwordregs[regedx] = 0x00000001;
                        }
                        break;
                    default:
                        printf("Error: Unknown 0x0F opcode 0x%02x\n", opcode);
                        PrintStatus();
                        exit_or_restart(1);
                        break;
                }
                break;

            case 0x10:    /* 10 ADC Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8 (reg);
                writerm8(rm, op_adc8());
                break;

            case 0x11:    /* 11 ADC Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16 (reg);
                writerm16(rm, op_adc16());
                break;

            case 0x12:    /* 12 ADC Gb Eb */
                modregrm();
                oper1b = getreg8 (reg);
                oper2b = readrm8(rm);
                putreg8 (reg, op_adc8());
                break;

            case 0x13:    /* 13 ADC Gv Ev */
                modregrm();
                oper1 = getreg16 (reg);
                oper2 = readrm16(rm);
                putreg16 (reg, op_adc16());
                break;

            case 0x14:    /* 14 ADC al Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = op_adc8();
                break;

            case 0x15:    /* 15 ADC eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                regs.wordregs[regax] = op_adc16();
                break;

            case 0x16:    /* 16 PUSH segregs[regss] */
                push16(segregs[regss]);
                break;

            case 0x17:    /* 17 POP segregs[regss] */
                segregs[regss] = pop16();
                break;

            case 0x18:    /* 18 SBB Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8 (reg);
                writerm8(rm, op_sbb8());
                break;

            case 0x19:    /* 19 SBB Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16 (reg);
                writerm16(rm, op_sbb16());
                break;

            case 0x1A:    /* 1A SBB Gb Eb */
                modregrm();
                oper1b = getreg8 (reg);
                oper2b = readrm8(rm);
                putreg8 (reg, op_sbb8());
                break;

            case 0x1B:    /* 1B SBB Gv Ev */
                modregrm();
                oper1 = getreg16 (reg);
                oper2 = readrm16(rm);
                putreg16 (reg, op_sbb16());
                break;

            case 0x1C:    /* 1C SBB regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = op_sbb8();
                break;

            case 0x1D:    /* 1D SBB eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                regs.wordregs[regax] = op_sbb16();
                break;

            case 0x1E:    /* 1E PUSH segregs[regds] */
                push16(segregs[regds]);
                break;

            case 0x1F:    /* 1F POP segregs[regds] */
                segregs[regds] = pop16();
                break;

            case 0x20:    /* 20 AND Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8 (reg);
                writerm8(rm, op_and8());
                break;

            case 0x21:    /* 21 AND Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                writerm16(rm, op_and16());
                break;

            case 0x22:    /* 22 AND Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                putreg8(reg, op_and8());
                break;

            case 0x23:    /* 23 AND Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                putreg16(reg, op_and16());
                break;

            case 0x24:    /* 24 AND regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                regs.byteregs[regal] = op_and8();
                break;

            case 0x25:    /* 25 AND eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
                regs.wordregs[regax] = op_and16();
                break;

            case 0x26:    /* segment segregs[reges] */
                useseg = segregs[reges];
                segoverride = 1;
                goto restart;
                break;

            case 0x27:    /* 27 DAA */
            {
                oldcf = cf;
                if (((regs.byteregs[regal] & 0x0Fu) > 9) || (af == 1)) {
                    oper1 = regs.byteregs[regal] + 6;
                    regs.byteregs[regal] = oper1 & 0xFFu;
                    cf = oldcf | ((oper1 & 0xFF00u) ? 1 : 0);
                    af = 1;
                } else {
                    af = 0;
                }

                if ((regs.byteregs[regal] > 0x99) || (oldcf == 1)) {
                    regs.byteregs[regal] += 0x60;
                    cf = 1;
                } else {
                    cf = 0;
                }

                flag_szp8(regs.byteregs[regal]);
                break;
            }

            case 0x28:    /* 28 SUB Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8(reg);
                writerm8(rm, op_sub8());
                break;

            case 0x29:    /* 29 SUB Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                writerm16(rm, op_sub16());
                break;

            case 0x2A:    /* 2A SUB Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                putreg8(reg, op_sub8());
                break;

            case 0x2B:    /* 2B SUB Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                putreg16(reg, op_sub16());
                break;

            case 0x2C:    /* 2C SUB regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                regs.byteregs[regal] = op_sub8();
                break;

            case 0x2D:    /* 2D SUB eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
                regs.wordregs[regax] = op_sub16();
                break;

            case 0x2E:    /* segment segregs[regcs] */
                useseg = segregs[regcs];
                segoverride = 1;
                goto restart;
                break;

            case 0x2F:    /* 2F DAS */
                // TODO wrong
                //printf("DAS executed\n");
                //PrintStatus();
                //exit_or_restart(1);
                if (((regs.byteregs[regal] & 0x0Fu) > 9) || (af == 1)) {
                    oper1 = regs.byteregs[regal] - 6;
                    regs.byteregs[regal] = oper1 & 0xFFu;
                    cf = (oper1 & 0xFF00u) ? 1 : 0;
                    af = 1;
                } else {
                    af = 0;
                }

                if (((regs.byteregs[regal] & 0xF0u) > 0x90) || (cf == 1)) {
                    regs.byteregs[regal] -= 0x60;
                    cf = 1;
                } else {
                    cf = 0;
                }
                flag_szp8(regs.byteregs[regal]);
                break;

            case 0x30:    /* 30 XOR Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8(reg);
                writerm8(rm, op_xor8());
                break;

            case 0x31:    /* 31 XOR Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                writerm16(rm, op_xor16());
                break;

            case 0x32:    /* 32 XOR Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                putreg8(reg, op_xor8());
                break;

            case 0x33:    /* 33 XOR Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                putreg16(reg, op_xor16());
                break;

            case 0x34:    /* 34 XOR regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = op_xor8();
                break;

            case 0x35:    /* 35 XOR eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                regs.wordregs[regax] = op_xor16();
                break;

            case 0x36:    /* segment segregs[regss] */
                useseg = segregs[regss];
                segoverride = 1;
                goto restart;
                break;

            case 0x37:    /* 37 AAA ASCII */
                //printf("AAA executed\n");
                if (((regs.byteregs[regal] & 0xFu) > 9) || (af == 1)) {
                    regs.byteregs[regal] = regs.byteregs[regal] + 6;
                    regs.byteregs[regah] = regs.byteregs[regah] + 1;
                    af = 1;
                    cf = 1;
                } else {
                    af = 0;
                    cf = 0;
                }
                regs.byteregs[regal] &= 0xFu;
                break;

            case 0x38:    /* 38 CMP Eb Gb */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = getreg8(reg);
                flag_sub8(oper1b, oper2b);
                break;

            case 0x39:    /* 39 CMP Ev Gv */
                modregrm();
                oper1 = readrm16(rm);
                oper2 = getreg16(reg);
                flag_sub16(oper1, oper2);
                break;

            case 0x3A:    /* 3A CMP Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                flag_sub8(oper1b, oper2b);
                break;

            case 0x3B:    /* 3B CMP Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                flag_sub16(oper1, oper2);
                break;

            case 0x3C:    /* 3C CMP regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                flag_sub8(oper1b, oper2b);
                break;

            case 0x3D:    /* 3D CMP eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                flag_sub16(oper1, oper2);
                break;

            case 0x3E:    /* segment segregs[regds] */
                useseg = segregs[regds];
                segoverride = 1;
                goto restart;
                break;

            case 0x3F:    /* 3F AAS ASCII */
                printf("AAS executed\n");
                PrintStatus();
                exit_or_restart(1);
                if (((regs.byteregs[regal] & 0xFu) > 9) || (af == 1)) {
                    regs.byteregs[regal] = regs.byteregs[regal] - 6;
                    regs.byteregs[regah] = regs.byteregs[regah] - 1;
                    af = 1;
                    cf = 1;
                } else {
                    af = 0;
                    cf = 0;
                }

                regs.byteregs[regal] = regs.byteregs[regal] & 0xFu;
                break;

            case 0x40:    // 40 INC eAX
            case 0x41:    // 41 INC eCX
            case 0x42:    // 42 INC eDX
            case 0x43:    // 43 INC eBX
            case 0x44:    // 44 INC eSP
            case 0x45:    // 45 INC eBP
            case 0x46:    // 46 INC eSI
            case 0x47:    // 47 INC eDI
                oldcf = cf;
                oper1 = getreg16(opcode & 7u);
                oper2 = 1;
                res16 = op_add16();
                cf = oldcf;
                putreg16(opcode & 7u, res16);
                break;

            case 0x48:    // 48 DEC eAX
            case 0x49:
            case 0x4A:
            case 0x4B:
            case 0x4C:
            case 0x4D:
            case 0x4E:
            case 0x4F:
                oldcf = cf;
                oper1 = getreg16(opcode & 7u);
                oper2 = 1;
                res16 = op_sub16();
                cf = oldcf;
                putreg16(opcode & 7u, res16);
                break;

            case 0x50:    // 50 PUSH eAX
            case 0x51:
            case 0x52:
            case 0x53:
            case 0x54:   // 54 PUSH eSP
            case 0x55:
            case 0x56:
            case 0x57:
                push16(getreg16(opcode & 7u));
                break;

            case 0x58:    // 58 POP eAX
            case 0x59:
            case 0x5A:
            case 0x5B:
            case 0x5C:
            case 0x5D:
            case 0x5E:
            case 0x5F:
                putreg16(opcode & 7u, pop16());
                break;

            case 0x60:   //  60 PUSHA (80186+)
            {
                uint16_t oldsp = regs.wordregs[regsp];
                push16(regs.wordregs[regax]);
                push16(regs.wordregs[regcx]);
                push16(regs.wordregs[regdx]);
                push16(regs.wordregs[regbx]);
                push16(oldsp);
                push16(regs.wordregs[regbp]);
                push16(regs.wordregs[regsi]);
                push16(regs.wordregs[regdi]);
                break;
            }
            case 0x61:    // 61 POPA (80186+)
                regs.wordregs[regdi] = pop16();
                regs.wordregs[regsi] = pop16();
                regs.wordregs[regbp] = pop16();
                pop16();
                regs.wordregs[regbx] = pop16();
                regs.wordregs[regdx] = pop16();
                regs.wordregs[regcx] = pop16();
                regs.wordregs[regax] = pop16();
                break;
/*
            case 0x62: // 62 BOUND Gv, Ev (80186+)
            {
                printf("BOUND executed\n");
                PrintStatus();
                exit_or_restart(1);
                modregrm();
                uint32_t ea = getea(rm);
                if (signext32(getreg16(reg)) < signext32(Read16Long(ea >> 4u, ea & 15u))) {
                    intcall86(5); //bounds check exception
                } else {
                    ea += 2;
                    if (signext32(getreg16(reg)) > signext32(Read16Long(ea >> 4u, ea & 15u))) {
                        intcall86(5); //bounds check exception
                    }
                }
                break;
            }
*/
            case 0x63: { // callback for the ROM and DOS functions
                // TODO, enable check
                //if (segregs[regcs] == 0xF000) {
                    ROMExec(Read8LongUnsafe(segregs[regcs], ip), 0x0);
                    AdvanceIP(1);
                //} else {
                //    printf("callback not in ROM\n");
                //    exit_or_restart(1);
                //}
                break;
            }

            case 0x66: { // callback for the ROM and DOS functions
                opcode = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                opcode66(opcode);
                break;
            }
/*
            case 0x67: // 32-bit address mode
                goto restart;
                break;
*/
/*
            case 0x68:    // 68 PUSH Iv (80186+)
                push(Read16LongUnsafe(segregs[regcs], ip));
                AdvanceIP(2);
                break;
*/
/*
            case 0x69:    // 69 IMUL Gv Ev Iv (80186+)
            {
                modregrm();
                //op_imul16(readrm16(rm), Read16LongUnsafe(segregs[regcs], ip));
                //AdvanceIP(2);
                int32_t temp1 = signext32(readrm16(rm));
                int32_t temp2 = signext32(Read16LongUnsafe(segregs[regcs], ip));
                AdvanceIP (2);
                int32_t temp3 = temp1 * temp2;
                putreg16 (reg, temp3 & 0xFFFFu);
                if (temp3 & 0xFFFF0000u) {
                    cf = 1;
                    of = 1;
                } else {
                    cf = 0;
                    of = 0;
                }

                break;
            }
*/
/*
            case 0x6A:    // 6A PUSH Ib (80186+)
                push(Read8LongUnsafe(segregs[regcs], ip));
                AdvanceIP(1);
                break;

            case 0x6B:    // 6B IMUL Gv Eb Ib (80186+)
            {
                modregrm();
                //op_imul16(readrm16(rm), signext16(Read8LongUnsafe(segregs[regcs], ip)));
                //AdvanceIP(1);

                int32_t temp1 = signext32(readrm16(rm));
                int32_t temp2 = signext32(signext16(Read8LongUnsafe(segregs[regcs], ip)));
                AdvanceIP(1);

                int32_t temp3 = temp1 * temp2;
                putreg16 (reg, temp3 & 0xFFFFu);
                if (temp3 & 0xFFFF0000u) {
                    cf = 1;
                    of = 1;
                } else {
                    cf = 0;
                    of = 0;
                }
                break;
            }
*/
/*
            case 0x6C:    // 6E INSB
                insb();
                break;

            case 0x6D:    // 6F INSW
                insw();
                break;

            case 0x6E:    // 6E OUTSB
                outsb();
                break;

            case 0x6F:    // 6F OUTSW
                outsw();
                break;
*/
            case 0x70:    // 70 JO Jb
                if (of) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x71:    /* 71 JNO Jb */
                if (!of) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x72:    /* 72 JB Jb */
                if (cf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x73:    /* 73 JNB Jb */
                if (!cf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x74:    /* 74 JZ Jb */
                if (zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x75:    /* 75 JNZ Jb */
                if (!zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x76:    /* 76 JBE Jb */
                if (cf || zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x77:    /* 77 JA Jb */
                if (!cf && !zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x78:    /* 78 JS Jb */
                if (sf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x79:    /* 79 JNS Jb */
                if (!sf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7A:    /* 7A JPE Jb */
                if (pf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7B:    /* 7B JPO Jb */
                if (!pf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7C:    /* 7C JL Jb */
                if (sf != of) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7D:    /* 7D JGE Jb */
                if (sf == of) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7E:    /* 7E JLE Jb */
                if ((sf != of) || zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x7F:    /* 7F JG Jb */
                if (!zf && (sf == of)) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0x80:    /* 80/82 GRP1 Eb Ib */
                modregrm();
                oper1b = readrm8(rm);
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                switch (reg) {
                    case 0:
                        res8 = op_add8();
                        break;
                    case 1:
                        res8 = op_or8();
                        break;
                    case 2:
                        res8 = op_adc8();
                        break;
                    case 3:
                        res8 = op_sbb8();
                        break;
                    case 4:
                        res8 = op_and8();
                        break;
                    case 5:
                        res8 = op_sub8();
                        break;
                    case 6:
                        res8 = op_xor8();
                        break;
                    case 7:
                        flag_sub8(oper1b, oper2b);
                        break;
                    default:
                        break;    /* to avoid compiler warnings */
                }

                if (reg < 7) {
                    writerm8(rm, res8);
                }
                break;

            case 0x81:    /* 81 GRP1 Ev Iv */
            case 0x83:    /* 83 GRP1 Ev Ib */
                modregrm();
                oper1 = readrm16(rm);
                if (opcode == 0x81) {
                    oper2 = Read16LongUnsafe(segregs[regcs], ip);
                    AdvanceIP (2);
                } else {
                    oper2 = signext16(Read8LongUnsafe(segregs[regcs], ip));
                    AdvanceIP (1);
                }

                switch (reg) {
                    case 0:
                        res16 = op_add16();
                        break;
                    case 1:
                        res16 = op_or16();
                        break;
                    case 2:
                        res16 = op_adc16();
                        break;
                    case 3:
                        res16 = op_sbb16();
                        break;
                    case 4:
                        res16 = op_and16();
                        break;
                    case 5:
                        res16 = op_sub16();
                        break;
                    case 6:
                        res16 = op_xor16();
                        break;
                    case 7:
                        flag_sub16(oper1, oper2);
                        break;
                    default:
                        break;    /* to avoid compiler warnings */
                }

                if (reg < 7) {
                    writerm16(rm, res16);
                }
                break;

            case 0x84:    /* 84 TEST Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                oper2b = readrm8(rm);
                flag_logic8(oper1b & oper2b);
                break;

            case 0x85:    /* 85 TEST Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                oper2 = readrm16(rm);
                flag_logic16(oper1 & oper2);
                break;

            case 0x86:    /* 86 XCHG Gb Eb */
                modregrm();
                oper1b = getreg8(reg);
                putreg8(reg, readrm8(rm));
                writerm8(rm, oper1b);
                break;

            case 0x87:    /* 87 XCHG Gv Ev */
                modregrm();
                oper1 = getreg16(reg);
                putreg16(reg, readrm16(rm));
                writerm16(rm, oper1);
                break;

            case 0x88:    /* 88 MOV Eb Gb */
                modregrm();
                writerm8(rm, getreg8(reg));
                break;

            case 0x89:    /* 89 MOV Ev Gv */
                modregrm();
                writerm16(rm, getreg16(reg));
                break;

            case 0x8A:    /* 8A MOV Gb Eb */
                modregrm();
                putreg8 (reg, readrm8(rm));
                break;

            case 0x8B:    /* 8B MOV Gv Ev */
                modregrm();
                putreg16 (reg, readrm16(rm));
                break;

            case 0x8C:    /* 8C MOV Ew Sw */
                modregrm();
                writerm16(rm, segregs[reg]);
                break;

            case 0x8D:    /* 8D LEA Gv M */
            {
                modregrm();
                uint32_t ea = getea(rm); // getea sets useseg, so be careful
                putreg16 (reg, ea - segbase(useseg));
                break;
            }

            case 0x8E:    /* 8E MOV Sw Ew */
                modregrm();
                segregs[reg] = readrm16(rm);
                break;

            case 0x8F:    /* 8F POP Ev */
                modregrm();
                writerm16(rm, pop16());
                break;

            case 0x90: // NOP (XCHG AX, AX)
            case 0x91: // XCHG
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97: {
                uint16_t oper1 = getreg16(opcode & 7);
                putreg16(opcode & 7, regs.wordregs[regax]);
                regs.wordregs[regax] = oper1;
                break;
            }
            case 0x98:    /* 98 CBW */
                regs.wordregs[regax] = signext16(regs.byteregs[regal]);
                break;

            case 0x99:    /* 99 CWD */
                regs.wordregs[regdx] = (regs.wordregs[regax] & 0x8000) ? 0xFFFF : 0x0000;
                break;

            case 0x9A:    /* 9A CALL Ap */
            {
                uint16_t newip = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
                uint16_t newcs = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP(2);
                push16(segregs[regcs]);
                push16(ip);
                ip = newip;
                segregs[regcs] = newcs;
                break;
            }
            case 0x9B:    /* 9B WAIT */
                break;

            case 0x9C:    /* 9C PUSHF */
                push16(makeflags());
                break;

            case 0x9D:    /* 9D POPF */
            decodeflags(pop16());
                break;

            case 0x9E:    /* 9E SAHF */
            decodeflags((makeflags() & 0xFF00u) | regs.byteregs[regah]);
                break;

            case 0x9F:    /* 9F LAHF */
                regs.byteregs[regah] = makeflags();
                break;

            case 0xA0:    /* A0 MOV regs.byteregs[regal] Ob */
                regs.byteregs[regal] = Read8Long(useseg, Read16LongUnsafe(segregs[regcs], ip));
                AdvanceIP(2);
                break;

            case 0xA1:    /* A1 MOV eAX Ov */
                regs.wordregs[regax] = Read16Long(useseg, Read16LongUnsafe(segregs[regcs], ip));;
                AdvanceIP(2);
                break;

            case 0xA2:    /* A2 MOV Ob regs.byteregs[regal] */
                Write8Long(useseg, Read16LongUnsafe(segregs[regcs], ip), regs.byteregs[regal]);
                AdvanceIP(2);
                break;

            case 0xA3:    /* A3 MOV Ov eAX */
                Write16Long(useseg, Read16LongUnsafe(segregs[regcs], ip), regs.wordregs[regax]);
                AdvanceIP(2);
                break;

            case 0xA4:    /* A4 MOVSB */
                movsb();
                break;

            case 0xA5:    /* A5 MOVSW */
                movsw();
                break;

            case 0xA6:    /* A6 CMPSB */
                cmpsb();
                break;

            case 0xA7:    /* A7 CMPSW */
                cmpsw();
                break;

            case 0xA8:    /* A8 TEST regs.byteregs[regal] Ib */
                oper1b = regs.byteregs[regal];
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                flag_logic8(oper1b & oper2b);
                break;

            case 0xA9:    /* A9 TEST eAX Iv */
                oper1 = regs.wordregs[regax];
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                flag_logic16(oper1 & oper2);
                break;

            case 0xAA:    /* AA STOSB */
                stosb();
                break;

            case 0xAB:    /* AB STOSW */
                stosw();
                break;

            case 0xAC:    /* AC LODSB */
                lodsb();
                break;

            case 0xAD:    /* AD LODSW */
                lodsw();
                break;

            case 0xAE:    /* AE SCASB */
                scasb();
                break;

            case 0xAF:    /* AF SCASW */
                scasw();
                break;

            case 0xB0: // B0 MOV regs.byteregs[regal] Ib
            case 0xB1:
            case 0xB2:
            case 0xB3:
            case 0xB4:
            case 0xB5:
            case 0xB6:
            case 0xB7:
                putreg8(opcode & 7u, Read8LongUnsafe(segregs[regcs], ip);)
                AdvanceIP (1);
                break;

            case 0xB8:    // B8 MOV eAX Iv
            case 0xB9:
            case 0xBA:
            case 0xBB:
            case 0xBC:
            case 0xBD:
            case 0xBE:
            case 0xBF:
                putreg16(opcode & 7u, Read16LongUnsafe(segregs[regcs], ip));
                AdvanceIP (2);
                break;

            case 0xC0:    // C0 GRP2 byte imm8 (80186+)
                modregrm();
                oper1b = readrm8(rm);
                oper2b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                writerm8(rm, op_grp2_8(oper2b));
                break;

            case 0xC1:    // C1 GRP2 word imm8 (80186+)
                modregrm();
                oper1 = readrm16(rm);
                oper2 = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                writerm16(rm, op_grp2_16((uint8_t) oper2));
                break;

            case 0xC2:    /* C2 RET Iw */
                oper1 = Read16Long(segregs[regcs], ip);
                AdvanceIP(2);
                ip = pop16();
                regs.wordregs[regsp] += oper1;
                break;

            case 0xC3:    /* C3 RET */
                ip = pop16();
                break;

            case 0xC4:    /* C4 LES Gv Mp */
            {
                modregrm();
                uint32_t ea = getea(rm);
                putreg16(reg, Read16(ea));
                segregs[reges] = Read16(ea + 2);
                break;
            }

            case 0xC5:    /* C5 LDS Gv Mp */
            {
                modregrm();
                uint32_t ea = getea(rm);
                putreg16(reg, Read16(ea));
                segregs[regds] = Read16(ea + 2);
                break;
            }

            case 0xC6:    /* C6 MOV Eb Ib */
                modregrm();
                writerm8(rm, Read8LongUnsafe(segregs[regcs], ip));
                AdvanceIP(1);
                break;

            case 0xC7:    /* C7 MOV Ev Iv */
                modregrm();
                writerm16(rm, Read16Long(segregs[regcs], ip));
                AdvanceIP(2);
                break;
/*
            case 0xC8:    // C8 ENTER (80186+)
            {
                uint16_t stacksize = Read16Long(segregs[regcs], ip);
                AdvanceIP (2);
                uint8_t nestlev = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                push(regs.wordregs[regbp]);
                uint16_t frametemp = regs.wordregs[regsp];
                if (nestlev) {
                    for (uint16_t temp16 = 1; temp16 < nestlev; temp16++) {
                        regs.wordregs[regbp] = regs.wordregs[regbp] - 2;
                        push(regs.wordregs[regbp]);
                    }

                    push(regs.wordregs[regsp]);
                }

                regs.wordregs[regbp] = frametemp;
                regs.wordregs[regsp] = regs.wordregs[regbp] - stacksize;
                break;
            }

            case 0xC9:    // C9 LEAVE (80186+)
                regs.wordregs[regsp] = regs.wordregs[regbp];
                regs.wordregs[regbp] = pop();
                break;
*/
            case 0xCA:    /* CA RETF Iw */
                oper1 = Read16Long(segregs[regcs], ip);
                AdvanceIP(2);
                ip = pop16();
                segregs[regcs] = pop16();
                regs.wordregs[regsp] += oper1;
                break;

            case 0xCB:    /* CB RETF */
                ip = pop16();
                segregs[regcs] = pop16();
                break;

            case 0xCC:    /* CC INT 3 */
                intcall86(3);
                break;

            case 0xCD:    /* CD INT Ib */
                oper1b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP(1);
                intcall86(oper1b);
                break;

            case 0xCE:    // CE INTO
                if (of) {
                    intcall86(4);
                }
                break;

            case 0xCF:    /* CF IRET */
                //printf("iret to 0x%04x:0x%04x\n", segregs[regcs], ip);
                ip = pop16();
                segregs[regcs] = pop16();
                decodeflags(pop16());
                break;

            case 0xD0:    /* D0 GRP2 Eb 1 */
                modregrm();
                oper1b = readrm8(rm);
                writerm8(rm, op_grp2_8(1));
                break;

            case 0xD1:    /* D1 GRP2 Ev 1 */
                modregrm();
                oper1 = readrm16(rm);
                writerm16(rm, op_grp2_16(1));
                break;

            case 0xD2:    /* D2 GRP2 Eb regs.byteregs[regcl] */
                modregrm();
                oper1b = readrm8(rm);
                writerm8(rm, op_grp2_8(regs.byteregs[regcl]));
                break;

            case 0xD3:    /* D3 GRP2 Ev regs.byteregs[regcl] */
                modregrm();
                oper1 = readrm16(rm);
                writerm16(rm, op_grp2_16(regs.byteregs[regcl]));
                break;

            case 0xD4:    /* D4 AAM I0 */
                oper1 = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                if (!oper1) {
                    printf("division by zero during executing AAM\n");
                    PrintStatus();
                    exit_or_restart(1);
                    intcall86(0);
                    break;
                }    /* division by zero */

                regs.byteregs[regah] = (regs.byteregs[regal] / oper1) & 0xFFu;
                regs.byteregs[regal] = (regs.byteregs[regal] % oper1) & 0xFFu;
                flag_szp8(regs.byteregs[regal]);
                break;

            case 0xD5:    /* D5 AAD I0 */
                oper1 = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = (regs.byteregs[regah] * oper1 + regs.byteregs[regal]) & 255;
                regs.byteregs[regah] = 0;
                flag_szp8(regs.byteregs[regal]);
                sf = 0;
                break;
/*
#ifndef CPU_NO_SALC
            case 0xD6:    // D6 XLAT on V20/V30, SALC on 8086/8088
                regs.byteregs[regal] = cf ? 0xFF : 0x00;
                break;
#endif
*/
            case 0xD7:    /* D7 XLAT */
                regs.byteregs[regal] = Read8Long(useseg, regs.wordregs[regbx] + regs.byteregs[regal]);
                break;
/*
            case 0xD8:
            case 0xD9:
            case 0xDA:
            case 0xDB:
            case 0xDC:
            case 0xDE:
            case 0xDD:
            case 0xDF:    // escape to x87 FPU (unsupported)
                modregrm();
                break;
*/
            case 0xE0:    /* E0 LOOPNZ Jb */
                regs.wordregs[regcx]--;
                if ((regs.wordregs[regcx]) && !zf) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0xE1:    /* E1 LOOPZ Jb */
                regs.wordregs[regcx]--;
                if (regs.wordregs[regcx] && (zf == 1)) {
                    ip += signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0xE2:    /* E2 LOOP Jb */
                regs.wordregs[regcx]--;
                if (regs.wordregs[regcx]) {
                    ip = ip + signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0xE3:    /* E3 JCXZ Jb */
                if (!regs.wordregs[regcx]) {
                    ip = ip + signext16(Read8LongUnsafe(segregs[regcs], ip));
                }
                AdvanceIP (1);
                break;

            case 0xE4:    /* E4 IN regs.byteregs[regal] Ib */
                oper1b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.byteregs[regal] = (uint8_t) portin(oper1b);
                break;

            case 0xE5:    /* E5 IN eAX Ib */
                oper1b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                regs.wordregs[regax] = portin16(oper1b);
                break;

            case 0xE6:    /* E6 OUT Ib regs.byteregs[regal] */
                oper1b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                portout(oper1b, regs.byteregs[regal]);
                break;

            case 0xE7:    /* E7 OUT Ib eAX */
                oper1b = Read8LongUnsafe(segregs[regcs], ip);
                AdvanceIP (1);
                portout16(oper1b, regs.wordregs[regax]);
                break;

            case 0xE8:    /* E8 CALL Jv */
                oper1 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                push16(ip);
                ip += oper1;
                break;

            case 0xE9:    /* E9 JMP Jv */
                oper1 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                ip += oper1;
                break;

            case 0xEA:    /* EA JMP Ap */
                oper1 = Read16LongUnsafe(segregs[regcs], ip);
                AdvanceIP (2);
                oper2 = Read16LongUnsafe(segregs[regcs], ip);
                ip = oper1;
                segregs[regcs] = oper2;
                break;

            case 0xEB:    /* EB JMP Jb */
                oper1 = signext16(Read8LongUnsafe(segregs[regcs], ip));
                AdvanceIP (1);
                ip += oper1;
                break;

            case 0xEC:    /* EC IN regs.byteregs[regal] regdx */
                oper1 = regs.wordregs[regdx];
                regs.byteregs[regal] = (uint8_t) portin(oper1);
                break;

            case 0xED:    /* ED IN eAX regdx */
                oper1 = regs.wordregs[regdx];
                regs.wordregs[regax] = portin16(oper1);
                break;

            case 0xEE:    /* EE OUT regdx regs.byteregs[regal] */
                oper1 = regs.wordregs[regdx];
                portout(oper1, regs.byteregs[regal]);
                break;

            case 0xEF:    /* EF OUT regdx eAX */
                oper1 = regs.wordregs[regdx];
                portout16(oper1, regs.wordregs[regax]);
                break;

            case 0xF0:    /* F0 LOCK */
                break;

            case 0xF2:    /* REPNE/REPNZ */
            case 0xF3:    /* REP/REPE/REPZ */
                op_rep(opcode - 0xF2); // 0 = REPNE, 1 = REP
                break;

            case 0xF4:    /* F4 HLT */
                hltstate = 1;
                //AdvanceIP(-1); // no, for some reason we are not allowed to do this. HLT must continue at some point
                break;

            case 0xF5:    /* F5 CMC */
                cf = cf ? 0 : 1;
                break;

            case 0xF6:    /* F6 GRP3a Eb */
                op_grp3_8();
                break;

            case 0xF7:    /* F7 GRP3b Ev */
                op_grp3_16();
                break;

            case 0xF8:    /* F8 CLC */
                cf = 0;
                break;

            case 0xF9:    /* F9 STC */
                cf = 1;
                break;

            case 0xFA:    /* FA CLI */
                ifl = 0;
                break;

            case 0xFB:    /* FB STI */
                ifl = 1;
                break;

            case 0xFC:    /* FC CLD */
                df = 0;
                break;

            case 0xFD:    /* FD STD */
                df = 1;
                break;

            case 0xFE:    /* FE GRP4 Eb */
            {
                modregrm();
                oper1b = readrm8(rm);
                oper2b = 1;
                uint8_t tempcf = cf;
                if (reg == 0) { // INC
                    res8 = oper1b + oper2b;
                    flag_add8(oper1b, oper2b);
                } else if (reg == 1) { // DEC
                    res8 = oper1b - oper2b;
                    flag_sub8(oper1b, oper2b);
                } else {
                    printf("unknown opcode inc dec\n");
                }
                cf = tempcf;
                writerm8(rm, res8);
                break;
            }

            case 0xFF:    /* FF GRP5 Ev */
                op_grp5();
                break;

            default:
                // be careful, cs might have changed already
                printf("Illegal opcode: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X @ %04X:%04X\n",
                       Read8Long(segregs[regcs], saveip),
                       Read8Long(segregs[regcs], saveip + 1),
                       Read8Long(segregs[regcs], saveip + 2),
                       Read8Long(segregs[regcs], saveip + 3),
                       Read8Long(segregs[regcs], saveip + 4),
                       Read8Long(segregs[regcs], saveip + 5),
                       segregs[regcs],
                       saveip);
                //(Read8Long(segregs[regcs], saveip + 3) >> 3u) & 7
                PrintStatus();
                exit_or_restart(1);
                // or treat as NOP
                intcall86(6); /* trip invalid opcode exception (this occurs on the 80186+, 8086/8088 CPUs treat them as NOPs. */
                /* technically they aren't exactly like NOPs in most cases, but for our purposes, that's accurate enough. */
                break;
        }
    }
}
