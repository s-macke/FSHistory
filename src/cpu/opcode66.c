uint32_t oper32_1, oper32_2;


static inline uint32_t op_grp2_32(uint8_t cnt) {

    uint64_t s;
    uint64_t shift;
    uint64_t oldcf;

    s = oper32_1;
    oldcf = cf;
    if (cnt == 0) return s; // TODO, is this valid for all instructions
    cnt &= 0x1F;

    switch (reg) {
        case 0: /* ROL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x80000000u) ? 1 : 0;
                s = s << 1u;
                s = s | cf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 31u) & 1u);
            }
            break;

        case 1: /* ROR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1u;
                s = (s >> 1u) | ((uint16_t) cf << 31u);
            }

            if (cnt == 1) {
                of = (s >> 31u) ^ ((s >> 30u) & 1u);
            }
            break;

        case 2: /* RCL r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = (s & 0x80000000u) ? 1 : 0;

                s = s << 1u;
                s = s | oldcf;
            }

            if (cnt == 1) {
                of = cf ^ ((s >> 31u) & 1u);
            }
            break;

        case 3: /* RCR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                oldcf = cf;
                cf = s & 1u;
                s = (s >> 1u) | (oldcf << 31u);
            }

            if (cnt == 1) {
                of = (s >> 31u) ^ ((s >> 30u) & 1u);
            }
            break;

        case 4:  // SHL r/m8
            for (shift = 1; shift <= cnt; shift++) {
                cf = (s & 0x80000000u) ? 1 : 0;
                s = (s << 1u) & 0xFFFFFFFFu;
            }
            if (cnt == 1) {
                of = (cf == ((s >> 31u) & 1)) ? 0 : 1;
            }
            flag_szp32((uint32_t) s);
            break;

        case 5: /* SHR r/m8 */
            if (cnt == 1) {
                of = (s & 0x80000000u) ? 1 : 0;
            }

            for (shift = 1; shift <= cnt; shift++) {
                cf = s & 1;
                s = s >> 1;
            }

            flag_szp32((uint32_t) s);
            break;

        case 7: /* SAR r/m8 */
            for (shift = 1; shift <= cnt; shift++) {
                uint32_t msb = s & 0x80000000;
                cf = s & 1;
                s = (s >> 1) | msb;
            }
            if (cnt == 1) {
                of = 0;
            }
            flag_szp32((uint32_t) s);
            break;

        default:
            printf("Error: unknown grp2_32 op\n");
            exit_or_restart(1);
            break;
    }
    return (uint32_t) s & 0xFFFFFFFF;
}



static inline uint32_t op_xor32() {
    uint32_t res32 = oper32_1 ^ oper32_2;
    flag_logic32(res32);
    return res32;
}

static inline uint16_t op_add32() {
    uint32_t res32 = oper32_1 + oper32_2;
    flag_add32(oper32_1, oper32_2);
    return res32;
}

static inline void opcode66(uint8_t opcode) {
    uint8_t oldcf;
    uint32_t res32;

    switch (opcode) {

        case 0xF: // 0F
            opcode = Read8LongUnsafe(segregs[regcs], ip);
            AdvanceIP(1);
            switch(opcode) {

                case 0xac: // shrld
                {
                    modregrm();
                    printf("shrld mode=%i reg=%i rm=%i disp16=%i\n", mode, reg, rm, disp16);
                    oper32_1 = getreg32(reg); // low
                    oper32_2 = readrm32(rm); // high
                    int cnt = Read8LongUnsafe(segregs[regcs], ip) & 0x1F;
                    AdvanceIP(1);
                    for (int shift = 1; shift <= cnt; shift++) {
                        oper32_2 >>= 1;
                        if (oper32_1&1) oper32_2 |= 0x80000000;
                        oper32_1 >>= 1;
                    }
                    writerm32(rm, oper32_2);
                    flag_logic32(oper32_2);
                    break;
                }

                case 0xbd: // bsr
                {
                    printf("bsr mode=%i reg=%i rm=%i disp16=%i\n", mode, reg, rm, disp16);
                    modregrm();
                    oper32_2 = readrm32(rm); // value

                    if (oper32_2 == 0) {
                        zf = 1;
                    } else {
                        zf = 0;
                        oper32_1 = 31;
                        while ((oper32_2 & 0x80000000)==0) { oper32_1--; oper32_2<<=1; }
                        putreg32(reg, oper32_1);
                    }
                    break;
                }

                default:
                    printf("Error: Unknown 0x66 0x0F opcode 0x%02x 0x%02x 0x%02x\n", opcode, Read8LongUnsafe(segregs[regcs], ip), Read8LongUnsafe(segregs[regcs], ip+1));
                    PrintStatus();
                    exit_or_restart(1);
                    break;
            }
            break;


        case 0x33:    /* 33 XOR Gv Ev */
            modregrm();
            oper32_1 = getreg32(reg);
            oper32_2 = readrm32(rm);
            putreg32(reg, op_xor32());
            break;

        case 0x35:    /* 35 XOR eAX Iv */
            oper32_1 = regs.dwordregs[regeax];
            oper32_2 = Read32LongUnsafe(segregs[regcs], ip);
            AdvanceIP (4);
            regs.dwordregs[regeax] = op_xor32();
            break;

        case 0x3D:    /* 3D CMP eAX Iv */
            oper32_1 = regs.dwordregs[regeax];
            oper32_2 = Read32LongUnsafe(segregs[regcs], ip);
            AdvanceIP (4);
            flag_sub32(oper32_1, oper32_2);
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
            oper32_1 = getreg32(opcode & 7u);
            oper32_2 = 1;
            res32 = op_add32();
            cf = oldcf;
            putreg32(opcode & 7u, res32);
            break;

        case 0x50:    // 50 PUSH eAX
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:   // 54 PUSH eSP
        case 0x55:
        case 0x56:
        case 0x57:
            push32(getreg32(opcode & 7u));
            break;

        case 0x58:    // 58 POP eAX
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
            putreg32(opcode & 7u, pop32());
            break;

        case 0x60:   //  60 PUSHAD
        {
            uint32_t oldsp = regs.dwordregs[regesp];
            push32(regs.dwordregs[regeax]);
            push32(regs.dwordregs[regecx]);
            push32(regs.dwordregs[regedx]);
            push32(regs.dwordregs[regebx]);
            push32(oldsp);
            push32(regs.dwordregs[regebp]);
            push32(regs.dwordregs[regesi]);
            push32(regs.dwordregs[regedi]);
            break;
        }

        case 0x61:    // 61 POPAD
            regs.dwordregs[regedi] = pop32();
            regs.dwordregs[regesi] = pop32();
            regs.dwordregs[regebp] = pop32();
            pop32();
            regs.dwordregs[regebx] = pop32();
            regs.dwordregs[regedx] = pop32();
            regs.dwordregs[regecx] = pop32();
            regs.dwordregs[regeax] = pop32();
            break;

        case 0x89:    /* 89 MOV Ev Gv */
            modregrm();
            writerm32(rm, getreg32(reg));
            break;

        case 0x8B:    /* 8B MOV Gv Ev */
            modregrm();
            printf("mov mode=%i reg=%i rm=%i disp16=%i\n", mode, reg, rm, disp16);
            printf("0x%08x\n", Read32Long(segregs[regcs], regs.wordregs[regsi]));
            putreg32(reg, readrm32(rm));
            break;

        case 0x9c: // pushd
            push32(makeflags());
            break;

        case 0x9D:    /* 9D POPF */
            decodeflags(pop32());
            break;

        case 0xC7:    /* C7 MOV Ev Iv */
            modregrm();
            writerm32(rm, Read32Long(segregs[regcs], ip));
            AdvanceIP(4);
            break;

        case 0xD3:    /* D3 GRP2 Ev regs.byteregs[regcl] */
            modregrm();
            oper32_1 = readrm32(rm);
            writerm32(rm, op_grp2_32(regs.byteregs[regcl]));
            break;


        default:
            printf("Error: Unknown 0x66 opcode 0x%02x\n", opcode);

            PrintStatus();

            exit_or_restart(1);
            break;
    }
}