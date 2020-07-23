
static inline void op_mul8(uint8_t oper1b, uint8_t oper2b) {
    uint32_t temp1 = (uint32_t)oper1b * (uint32_t)oper2b;
    regs.wordregs[regax] = temp1;
    //flag_szp16((uint16_t) temp1);
    if (regs.byteregs[regah]) {
        cf = 1;
        of = 1;
    } else {
        cf = 0;
        of = 0;
    }
    zf = 0;
}

static inline void op_imul8(uint8_t oper1b, uint8_t oper2b) {
    int32_t temp1 = signext32(signext16(oper1b));
    int32_t temp2 = signext32(signext16(oper2b));

    int32_t temp3 = (temp1 * temp2) & 0xFFFF;
    regs.wordregs[regax] = temp3;

    if (regs.byteregs[regah]) {
        cf = 1;
        of = 1;
    } else {
        cf = 0;
        of = 0;
    }
    zf = 0;
}

static inline void op_mul16(uint16_t oper1, uint16_t oper2) {
    uint32_t temp1 = (uint32_t)oper1 * (uint32_t)oper2;

    regs.wordregs[regax] = temp1 & 0xFFFF;
    regs.wordregs[regdx] = temp1 >> 16;

    if (regs.wordregs[regdx]) {
        cf = 1;
        of = 1;
    } else {
        cf = 0;
        of = 0;
    }
    zf = 0;
}

static inline void op_imul16(uint16_t oper1, uint16_t oper2) {
    int32_t temp1 = signext32(oper1);
    int32_t temp2 = signext32(oper2);

    int32_t temp3 = temp1 * temp2;

    regs.wordregs[regax] = temp3 & 0xFFFF;
    regs.wordregs[regdx] = (temp3 >> 16) & 0xFFFF;
    if (regs.wordregs[regdx]) {
        cf = 1;
        of = 1;
    } else {
        cf = 0;
        of = 0;
    }
    zf = 0;
}

static inline void op_div8(uint16_t valdiv, uint8_t divisor) {
    if (divisor == 0) {
        intcall86(0);
        return;
    }

    if ((valdiv / (uint16_t) divisor) > 0xFF) {
        intcall86(0);
        return;
    }

    regs.byteregs[regah] = valdiv % (uint16_t) divisor;
    regs.byteregs[regal] = valdiv / (uint16_t) divisor;
}

static inline void op_idiv8(uint16_t valdiv, uint8_t divisor) {
    if (divisor == 0) {
        intcall86(0);
        return;
    }

    int16_t temp = (int16_t) valdiv / signext16(divisor);

    if (temp > 0x7F || temp < -0x7F) {
        intcall86(0);
        return;
    }
    regs.byteregs[regah] = (int16_t) valdiv % signext16(divisor);
    regs.byteregs[regal] = temp;
}

static inline void op_div16(uint32_t valdiv, uint16_t divisor) {
    if (divisor == 0) {
        intcall86(0);
        return;
    }
    //printf("div %i %i\n", valdiv, divisor);
    if ((valdiv / (uint32_t) divisor) > 0xFFFF) {
        intcall86(0);
        return;
    }

    regs.wordregs[regdx] = valdiv % (uint32_t) divisor;
    regs.wordregs[regax] = valdiv / (uint32_t) divisor;
}

static inline void op_idiv16(int32_t valdiv, uint16_t divisor) {
    if (divisor == 0) {
        intcall86(0);
        return;
    }

    int32_t temp = (int32_t) valdiv / signext32(divisor);

    if (temp > 0x7FFF || temp < -0x7FFF) {
        intcall86(0);
        return;
    }

    regs.wordregs[regdx] = (int32_t) valdiv % signext32(divisor);
    regs.wordregs[regax] = temp;
}

static inline void flag_adc8(uint8_t v1, uint8_t v2, uint8_t v3) {
    /* v1 = destination operand, v2 = source operand, v3 = carry flag */
    uint16_t dst = (uint16_t) v1 + (uint16_t) v2 + (uint16_t) v3;

    flag_szp8((uint8_t) dst);
    cf = (dst & 0xFF00)?1:0;
    of = ((dst ^ v1) & (dst ^ v2) & 0x80)?1:0;
    af = (((v1 ^ v2 ^ dst) & 0x10) == 0x10)?1:0;
}

static void inline flag_adc16(uint16_t v1, uint16_t v2, uint16_t v3) {
    uint32_t dst = (uint32_t) v1 + (uint32_t) v2 + (uint32_t) v3;

    flag_szp16((uint16_t) dst);
    cf = (dst & 0xFFFF0000)?1:0;
    of = (((dst ^ v1) & (dst ^ v2)) & 0x8000)?1:0;
    af = (((v1 ^ v2 ^ dst) & 0x10) == 0x10)?1:0;
}

static void inline flag_add8(uint8_t v1, uint8_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint16_t dst = (uint16_t) v1 + (uint16_t) v2;

    flag_szp8((uint8_t) dst);
    cf = (dst & 0xFF00)?1:0; /* set or clear carry flag */
    of = ((dst ^ v1) & (dst ^ v2) & 0x80)?1:0;
    af = (((v1 ^ v2 ^ dst) & 0x10) == 0x10)?1:0;
}

static inline void flag_add16(uint16_t v1, uint16_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint32_t dst = (uint32_t)v1 + (uint32_t)v2;

    flag_szp16((uint16_t) dst);
    cf = (dst & 0xFFFF0000)?1:0; /* set or clear carry flag */
    of = ((dst ^ v1) & (dst ^ v2) & 0x8000u)?1:0;
    af = (((v1 ^ v2 ^ dst) & 0x10u) == 0x10)?1:0;
}

static inline void flag_add32(uint32_t v1, uint32_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint64_t dst = (uint64_t)v1 + (uint64_t)v2;

    flag_szp32((uint32_t) dst);
    cf = (dst & 0xFFFFFFFF00000000l)?1:0; /* set or clear carry flag */
    of = ((dst ^ v1) & (dst ^ v2) & 0x80000000u)?1:0;
    af = (((v1 ^ v2 ^ dst) & 0x10u) == 0x10)?1:0;
}


static inline void flag_sbb8(uint8_t v1, uint16_t v2, uint8_t v3) {
    /* v1 = destination operand, v2 = source operand, v3 = carry flag */
    v2 += v3;
    uint16_t dst = (uint16_t)v1 - v2;

    flag_szp8((uint8_t) dst);
    cf = (dst & 0x8000u)?1:0;
    of = ((dst ^ v1) & (v1 ^ v2) & 0x80u)?1:0;
    af = ((v1 ^ v2 ^ dst) & 0x10u)?1:0;
}

static inline void flag_sbb16(uint16_t v1, uint32_t v2, uint16_t v3) {
    /* v1 = destination operand, v2 = source operand, v3 = carry flag */
    v2 += v3;
    uint32_t dst = (uint32_t)v1 - v2;

    flag_szp16((uint16_t)dst);
    cf = (dst & 0x80000000)?1:0;
    of = ((dst ^ v1) & (v1 ^ v2) & 0x8000u)?1:0;
    af= ((v1 ^ v2 ^ dst) & 0x10u)?1:0;
}

static inline void flag_sub8(uint8_t v1, uint8_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint16_t dst = (uint16_t)v1 - (uint16_t)v2;

    flag_szp8((uint8_t) dst);
    cf = (dst & 0x8000u) ? 1 : 0;
    of = ((dst ^ v1) & (v1 ^ v2) & 0x80u)?1:0;
    af = ((v1 ^ v2 ^ dst) & 0x10u)?1:0;
}

static inline void flag_sub16(uint16_t v1, uint16_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint32_t dst = (uint32_t)v1 - (uint32_t)v2;

    flag_szp16((uint16_t) dst);
    cf = (dst & 0x80000000) ? 1 : 0;
    of = ((dst ^ v1) & (v1 ^ v2) & 0x8000u)?1:0;
    af = ((v1 ^ v2 ^ dst) & 0x10u)?1:0;
}


static inline void flag_sub32(uint32_t v1, uint32_t v2) {
    /* v1 = destination operand, v2 = source operand */
    uint64_t dst = (uint64_t)v1 - (uint64_t)v2;

    flag_szp32((uint32_t) dst);
    cf = (dst & 0x8000000000000000) ? 1 : 0;
    of = ((dst ^ v1) & (v1 ^ v2) & 0x80000000u)?1:0;
    af = ((v1 ^ v2 ^ dst) & 0x10u)?1:0;
}

