
#define makeflags() \
    ( \
    ((uint32_t)cf    <<  0u) | \
    ((uint32_t)2u          ) | \
    ((uint32_t)pf    <<  2u) | \
    ((uint32_t)af    <<  4u) | \
    ((uint32_t)zf    <<  6u) | \
    ((uint32_t)sf    <<  7u) | \
    ((uint32_t)tf    <<  8u) | \
    ((uint32_t)ifl   <<  9u) | \
    ((uint32_t)df    << 10u) | \
    ((uint32_t)of    << 11u) | \
    ((uint32_t)ioplf << 12u) | \
    ((uint32_t)ntf   << 14u)  | \
    ((uint32_t)acf   << 18u) )

#define decodeflags(x) { \
    uint32_t temp32 = x; \
    cf = (temp32 >> 0u) & 1u; \
    pf = (temp32 >> 2u) & 1u; \
    af = (temp32 >> 4u) & 1u; \
    zf = (temp32 >> 6u) & 1u; \
    sf = (temp32 >> 7u) & 1u; \
    tf = (temp32 >> 8u) & 1u; \
    ifl = (temp32 >> 9u) & 1u; \
    df = (temp32 >> 10u) & 1u; \
    of = (temp32 >> 11u) & 1u; \
    ioplf = (temp32 >> 12u) & 3u; \
    ntf = (temp32 >> 14u) & 1u; \
    acf = (temp32 >> 18u) & 1u; \
    }

// Helper routines
uint32_t getflags() {
    return makeflags();
}

void setflags(uint32_t x) {
    decodeflags(x);
}

void SetCFInInterrupt() {
    uint8_t f = Read8Long(segregs[regss], regs.wordregs[regsp] + 4);
    Write8Long(segregs[regss], regs.wordregs[regsp] + 4, f | 1u);
}

void ClearCFInInterrupt() {
    uint8_t f = Read8Long(segregs[regss], regs.wordregs[regsp] + 4);
    Write8Long(segregs[regss], regs.wordregs[regsp] + 4, f & (~1u));
}

void SetZFInInterrupt() {
    uint8_t f = Read8Long(segregs[regss], regs.wordregs[regsp] + 4);
    Write8Long(segregs[regss], regs.wordregs[regsp] + 4, f | (1u << 6));
}

void ClearZFInInterrupt() {
    uint8_t f = Read8Long(segregs[regss], regs.wordregs[regsp] + 4);
    Write8Long(segregs[regss], regs.wordregs[regsp] + 4, f & (~(1 << 6u)));
}

static inline void flag_szp8(uint8_t value) {
    zf = value == 0 ? 1 : 0;
    sf = (value & 0x80u) ? 1 : 0;
    pf = parity[value]; /* retrieve parity state from lookup table */
}

static inline void flag_szp16(uint16_t value) {
    zf = value == 0 ? 1 : 0;
    sf = (value & 0x8000u) ? 1 : 0; /* set or clear sign flag */
    pf = parity[value & 0xFFu];  /* retrieve parity state from lookup table */
}

static inline void flag_szp32(uint32_t value) {
    zf = value == 0 ? 1 : 0;
    sf = (value & 0x80000000u) ? 1 : 0; /* set or clear sign flag */
    pf = parity[value & 0xFFu];  /* retrieve parity state from lookup table */
}

static inline void flag_logic8(uint8_t value) {
    flag_szp8(value);
    cf = 0;
    of = 0; /* bitwise logic ops always clear carry and overflow */
}

static inline void flag_logic16(uint16_t value) {
    flag_szp16(value);
    cf = 0;
    of = 0; /* bitwise logic ops always clear carry and overflow */
}

static inline void flag_logic32(uint32_t value) {
    flag_szp32(value);
    cf = 0;
    of = 0; /* bitwise logic ops always clear carry and overflow */
}
