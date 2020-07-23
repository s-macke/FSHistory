
inline static void movsb() {

    Write8Long(segregs[reges], regs.wordregs[regdi], Read8Long(useseg, regs.wordregs[regsi]));
    if (df) {
        regs.wordregs[regsi]--;
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regsi]++;
        regs.wordregs[regdi]++;
    }
}

inline static void movsw() {

    Write16Long(segregs[reges], regs.wordregs[regdi], Read16Long(useseg, regs.wordregs[regsi]));
    if (df) {
        regs.wordregs[regsi] -= 2;
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regsi] += 2;
        regs.wordregs[regdi] += 2;
    }
}

inline static void cmpsb() {

    oper1b = Read8Long(useseg, regs.wordregs[regsi]);
    oper2b = Read8Long(segregs[reges], regs.wordregs[regdi]);
    if (df) {
        regs.wordregs[regsi]--;
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regsi]++;
        regs.wordregs[regdi]++;
    }

    flag_sub8(oper1b, oper2b);
}

inline static void cmpsw() {

    oper1 = Read16Long(useseg, regs.wordregs[regsi]);
    oper2 = Read16Long(segregs[reges], regs.wordregs[regdi]);
    if (df) {
        regs.wordregs[regsi] -= 2;
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regsi] += 2;
        regs.wordregs[regdi] += 2;
    }

    flag_sub16(oper1, oper2);
}

inline static void stosb() {

    Write8Long(segregs[reges], regs.wordregs[regdi], regs.byteregs[regal]);
    if (df) {
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regdi]++;
    }
}

inline static void stosw() {

    Write16Long(segregs[reges], regs.wordregs[regdi], regs.wordregs[regax]);
    if (df) {
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regdi] += 2;
    }

}

inline static void lodsb() {

    regs.byteregs[regal] = Read8Long(useseg, regs.wordregs[regsi]);
    if (df) {
        regs.wordregs[regsi]--;
    } else {
        regs.wordregs[regsi]++;
    }
}

inline static void lodsw() {
    oper1 = Read16Long(useseg, regs.wordregs[regsi]);
    regs.wordregs[regax] = oper1;
    if (df) {
        regs.wordregs[regsi] -= 2;
    } else {
        regs.wordregs[regsi] += 2;
    }
}

inline static void scasb() {
    oper1b = regs.byteregs[regal];
    oper2b = Read8Long(segregs[reges], regs.wordregs[regdi]);
    //printf("scasb al=0x%02x with value 0x%02x 0x%04x:0x%04x\n", oper1b, oper2b, segregs[reges], regs.wordregs[regdi]);

    flag_sub8(oper1b, oper2b);
    if (df) {
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regdi]++;
    }
}

inline static void scasw() {

    oper1 = regs.wordregs[regax];
    oper2 = Read16Long(segregs[reges], regs.wordregs[regdi]);
    flag_sub16(oper1, oper2);
    if (df) {
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regdi] += 2;
    }
}

inline static void insb() {

    Write8Long(useseg, regs.wordregs[regsi], portin(regs.wordregs[regdx]));
    if (df) {
        regs.wordregs[regsi]--;
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regsi]++;
        regs.wordregs[regdi]++;
    }
}

inline static void insw() {

    Write16Long(useseg, regs.wordregs[regsi], portin16(regs.wordregs[regdx]));
    if (df) {
        regs.wordregs[regsi] -= 2;
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regsi] += 2;
        regs.wordregs[regdi] += 2;
    }
}

inline static void outsb() {

    portout(regs.wordregs[regdx], Read8Long(useseg, regs.wordregs[regsi]));
    if (df) {
        regs.wordregs[regsi]--;
        regs.wordregs[regdi]--;
    } else {
        regs.wordregs[regsi]++;
        regs.wordregs[regdi]++;
    }
}

inline static void outsw() {

    portout16(regs.wordregs[regdx], Read16Long(useseg, regs.wordregs[regsi]));
    if (df) {
        regs.wordregs[regsi] -= 2;
        regs.wordregs[regdi] -= 2;
    } else {
        regs.wordregs[regsi] += 2;
        regs.wordregs[regdi] += 2;
    }
}

static void op_rep(uint8_t reptype) {

    if (useseg != segregs[regds]) {
        printf("useseg in rep\n"); // we have to check where useseg is allowed
        exit_or_restart(1);
    }
    if (regs.wordregs[regcx] == 0) {
        AdvanceIP(1);
        return;
    }
    //printf("reptype = %i\n", reptype);
    uint8_t opcode = Read8LongUnsafe(segregs[regcs], ip);
    AdvanceIP(1);
    //printf("rep 0x%02x\n", opcode);
    switch (opcode) {
        case 0xA4:    /* A4 MOVSB */
            movsb();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xA5:    /* A5 MOVSW */
            movsw();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xA6:    /* A6 CMPSB */
            cmpsb();
            regs.wordregs[regcx]--;
            if ((reptype == 1) && !zf) {
                return;
            } else if ((reptype == 0) && (zf == 1)) {
                return;
            }
            AdvanceIP(-2);
            break;

        case 0xA7:    /* A7 CMPSW */
            cmpsw();
            regs.wordregs[regcx]--;
            if ((reptype == 1) && !zf) {
                return;
            } else if ((reptype == 0) && (zf == 1)) {
                return;
            }
            AdvanceIP(-2);
            break;

        case 0xAA:    /* AA STOSB */
            stosb();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xAB:    /* AB STOSW */
            stosw();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xAC:    /* AC LODSB */
            lodsb();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xAD:    /* AD LODSW */
            lodsw();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0xAE:    /* AE SCASB */
            scasb();
            regs.wordregs[regcx]--;
            if ((reptype == 1) && !zf) {
                return;
            } else if ((reptype == 0) && (zf == 1)) {
                return;
            }
            AdvanceIP(-2);
            break;

        case 0xAF:    /* AF SCASW */
            scasw();
            regs.wordregs[regcx]--;
            if ((reptype == 1) && !zf) {
                return;
            } else if ((reptype == 0) && (zf == 1)) {
                return;
            }
            AdvanceIP(-2);
            break;

        case 0x6C:    /* 6E INSB */
            insb();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0x6D:    /* 6F INSW */
            insw();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0x6E:    /* 6E OUTSB */
            outsb();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        case 0x6F:    /* 6F OUTSW */
            outsw();
            regs.wordregs[regcx]--;
            AdvanceIP(-2);
            break;

        default:
            printf("Illegal opcode after rep\n");
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}
