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

#ifndef DISASM_H
#define DISASM_H

#include "mytypes.h"

static unsigned decode_br8(BYTE *, unsigned, char *);
static unsigned decode_r8b(BYTE *, unsigned, char *);
static unsigned decode_wr16(BYTE *, unsigned, char *);
static unsigned decode_r16w(BYTE *, unsigned, char *);
static unsigned decode_ald8(BYTE *, unsigned, char *);
static unsigned decode_axd16(BYTE *, unsigned, char *);
static unsigned decode_pushpopseg(BYTE *, unsigned, char *);
static unsigned decode_databyte(BYTE *, unsigned, char *);
static unsigned decode_wordreg(BYTE *, unsigned, char *);
static unsigned decode_cond_jump(BYTE *, unsigned, char *);
static unsigned decode_bd8(BYTE *, unsigned, char *);
static unsigned decode_wd16(BYTE *, unsigned, char *);
static unsigned decode_wd8(BYTE *, unsigned, char *);
static unsigned decode_ws(BYTE *, unsigned, char *);
static unsigned decode_sw(BYTE *, unsigned, char *);
static unsigned decode_w(BYTE *, unsigned, char *);
static unsigned decode_string(BYTE *, unsigned, char *);
static unsigned decode_xchgax(BYTE *, unsigned, char *);
static unsigned decode_far(BYTE *, unsigned, char *);
static unsigned decode_almem(BYTE *, unsigned, char *);
static unsigned decode_axmem(BYTE *, unsigned, char *);
static unsigned decode_memal(BYTE *, unsigned, char *);
static unsigned decode_memax(BYTE *, unsigned, char *);
static unsigned decode_rd(BYTE *, unsigned, char *);
static unsigned decode_d16(BYTE *, unsigned, char *);
static unsigned decode_int3(BYTE *, unsigned, char *);
static unsigned decode_d8(BYTE *, unsigned, char *);
static unsigned decode_bbit1(BYTE *, unsigned, char *);
static unsigned decode_wbit1(BYTE *, unsigned, char *);
static unsigned decode_bbitcl(BYTE *, unsigned, char *);
static unsigned decode_wbitcl(BYTE *, unsigned, char *);
static unsigned decode_escape(BYTE *, unsigned, char *);
static unsigned decode_disp(BYTE *, unsigned, char *);
static unsigned decode_adjust(BYTE *, unsigned, char *);
static unsigned decode_d8al(BYTE *, unsigned, char *);
static unsigned decode_d8ax(BYTE *, unsigned, char *);
static unsigned decode_axd8(BYTE *, unsigned, char *);
static unsigned decode_disp16(BYTE *, unsigned, char *);
static unsigned decode_far_ind(BYTE *, unsigned, char *);
static unsigned decode_portdx(BYTE *, unsigned, char *);
static unsigned decode_f6(BYTE *, unsigned, char *);
static unsigned decode_f7(BYTE *, unsigned, char *);
static unsigned decode_b(BYTE *, unsigned, char *);
static unsigned decode_ff(BYTE *, unsigned, char *);
static unsigned decode_bioscall(BYTE *, unsigned, char *);

static char *byte_reg[] = { "al","cl","dl","bl","ah","ch","dh","bh" };
static char *word_reg[] = { "ax","cx","dx","bx","sp","bp","si","di" };
static char *seg_reg[] = { "es","cs","ss","ds", "unknown_seg_reg", "unknown_seg_reg", "unknown_seg_reg", "unknown_seg_reg" };
static char *index_reg[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };
static char *nul_reg[] = { "??", "??", "??", "??", "??", "??", "??", "??" };
static char *condition[] =
{
    "o","no","b","ae","z","nz","be","a","s","ns","p","np","l","ge","le","g"
};

static char *itext[] =
{ 
    "", "add", "push", "pop", "or", "adc", "sbb", "and",
    "es:", "daa", "sub", "cs:", "das", "xor", "ss:", "aaa", "cmp", "ds:",
    "aas", "inc", "dec", "j", "test", "xchg", "mov", "lea", "nop", "cbw",
    "cwd", "call", "wait", "pushf", "popf", "sahf", "lahf", "movs",
    "cmps", "stos", "lods", "scas", "ret", "les", "lds", "retf", "int",
    "into", "iret", "rol", "ror", "rcl", "rcr", "shl", "shr", "sar", "aam",
    "aad", "xlat", "esc", "loopne", "loope", "loop", "jcxz", "in", "out",
    "jmp", "lock", "repz", "repnz", "hlt", "cmc", "not", "neg", "mul", "imul",
    "div", "idiv", "clc", "stc", "cli", "sti", "cld", "std", "db", "???", "32:"
};

enum instructions
{
    none, add, push, pop, or, adc, sbb, and, es, daa,
    sub, cs, das, xor, ss, aaa, cmp, ds, aas, inc, dec, jump, test, xchg,
    mov, lea, nop, cbw, cwd, call, wait, pushf, popf, sahf, lahf, movs,
    cmps, stos, lods, scas, ret, les, lds, retf, intr, into, iret, rol,
    ror, rcl, rcr, shl, shr, sar, aam, aad, xlat, esc, loopne, loope, loop,
    jcxz, in, out, jmp, lock, repz, repnz, hlt, cmc, not, neg, mul, imul,
    divide, idiv, clc, stc, cli, sti, cld, std, db, illegal, bit32
};

#define DF_PREFIX  1
#define DF_NOSPACE 2

static int table_8x[] = { add, or, adc, sbb, and, sub, xor, cmp };
static int table_dx[] = { rol, ror, rcl, rcr, shl, shr, shl, sar };
static int table_f67[] = { test, illegal, not, neg, mul, imul, divide, idiv };
static int table_fe[] = { inc, dec, illegal, illegal, illegal, illegal, illegal, illegal };
static int table_ff[] = { inc, dec, call, call, jmp, jmp, push, illegal };

static struct Disasm
{
    int text;
    unsigned (*type)(BYTE *, unsigned, char *);
    int flags;
    int *supp;
} disasm_table[256] =
{
{ add, decode_br8 },            /* 0x00 */
{ add, decode_wr16 },           /* 0x01 */
{ add, decode_r8b },            /* 0x02 */
{ add, decode_r16w },           /* 0x03 */
{ add, decode_ald8 },           /* 0x04 */
{ add, decode_axd16 },          /* 0x05 */
{ push,decode_pushpopseg },     /* 0x06 */
{ pop, decode_pushpopseg },     /* 0x07 */
{ or,  decode_br8 },            /* 0x08 */
{ or,  decode_wr16 },           /* 0x09 */
{ or,  decode_r8b },            /* 0x0a */
{ or,  decode_r16w },           /* 0x0b */
{ or,  decode_ald8 },           /* 0x0c */
{ or,  decode_axd16 },          /* 0x0d */
{ push,decode_pushpopseg },     /* 0x0e */
{ db,  decode_databyte },       /* 0x0f */
{ adc, decode_br8 },            /* 0x10 */
{ adc, decode_wr16 },           /* 0x11 */
{ adc, decode_r8b },            /* 0x12 */
{ adc, decode_r16w },           /* 0x13 */
{ adc, decode_ald8 },           /* 0x14 */
{ adc, decode_axd16 },          /* 0x15 */
{ push,decode_pushpopseg },     /* 0x16 */
{ pop, decode_pushpopseg },     /* 0x17 */
{ sbb, decode_br8 },            /* 0x18 */
{ sbb, decode_wr16 },           /* 0x19 */
{ sbb, decode_r8b },            /* 0x1a */
{ sbb, decode_r16w },           /* 0x1b */
{ sbb, decode_ald8 },           /* 0x1c */
{ sbb, decode_axd16 },          /* 0x1d */
{ push,decode_pushpopseg },     /* 0x1e */
{ pop, decode_pushpopseg },     /* 0x1f */
{ and, decode_br8 },            /* 0x20 */
{ and, decode_wr16 },           /* 0x21 */
{ and, decode_r8b },            /* 0x22 */
{ and, decode_r16w },           /* 0x23 */
{ and, decode_ald8 },           /* 0x24 */
{ and, decode_axd16 },          /* 0x25 */
{ es,  NULL, DF_PREFIX },       /* 0x26 */
{ daa },                        /* 0x27 */
{ sub, decode_br8 },            /* 0x28 */
{ sub, decode_wr16 },           /* 0x29 */
{ sub, decode_r8b },            /* 0x2a */
{ sub, decode_r16w },           /* 0x2b */
{ sub, decode_ald8 },           /* 0x2c */
{ sub, decode_axd16 },          /* 0x2d */
{ cs,  NULL, DF_PREFIX },       /* 0x2e */
{ das },                        /* 0x2f */
{ xor, decode_br8 },            /* 0x30 */
{ xor, decode_wr16 },           /* 0x31 */
{ xor, decode_r8b },            /* 0x32 */
{ xor, decode_r16w },           /* 0x33 */
{ xor, decode_ald8 },           /* 0x34 */
{ xor, decode_axd16 },          /* 0x35 */
{ ss,  NULL, DF_PREFIX },       /* 0x36 */
{ aaa },                        /* 0x37 */
{ cmp, decode_br8 },            /* 0x38 */
{ cmp, decode_wr16 },           /* 0x39 */
{ cmp, decode_r8b },            /* 0x3a */
{ cmp, decode_r16w },           /* 0x3b */
{ cmp, decode_ald8 },           /* 0x3c */
{ cmp, decode_axd16 },          /* 0x3d */
{ ds,  NULL, DF_PREFIX },       /* 0x3e */
{ aas },                        /* 0x3f */
{ inc, decode_wordreg },        /* 0x40 */
{ inc, decode_wordreg },        /* 0x41 */
{ inc, decode_wordreg },        /* 0x42 */
{ inc, decode_wordreg },        /* 0x43 */
{ inc, decode_wordreg },        /* 0x44 */
{ inc, decode_wordreg },        /* 0x45 */
{ inc, decode_wordreg },        /* 0x46 */
{ inc, decode_wordreg },        /* 0x47 */
{ dec, decode_wordreg },        /* 0x48 */
{ dec, decode_wordreg },        /* 0x49 */
{ dec, decode_wordreg },        /* 0x4a */
{ dec, decode_wordreg },        /* 0x4b */
{ dec, decode_wordreg },        /* 0x4c */
{ dec, decode_wordreg },        /* 0x4d */
{ dec, decode_wordreg },        /* 0x4e */
{ dec, decode_wordreg },        /* 0x4f */
{ push,decode_wordreg },        /* 0x50 */
{ push,decode_wordreg },        /* 0x51 */
{ push,decode_wordreg },        /* 0x52 */
{ push,decode_wordreg },        /* 0x53 */
{ push,decode_wordreg },        /* 0x54 */
{ push,decode_wordreg },        /* 0x55 */
{ push,decode_wordreg },        /* 0x56 */
{ push,decode_wordreg },        /* 0x57 */
{ pop, decode_wordreg },        /* 0x58 */
{ pop, decode_wordreg },        /* 0x59 */
{ pop, decode_wordreg },        /* 0x5a */
{ pop, decode_wordreg },        /* 0x5b */
{ pop, decode_wordreg },        /* 0x5c */
{ pop, decode_wordreg },        /* 0x5d */
{ pop, decode_wordreg },        /* 0x5e */
{ pop, decode_wordreg },        /* 0x5f */
{ db,  decode_databyte },       /* 0x60 */
{ db,  decode_databyte },       /* 0x61 */
{ db,  decode_databyte },       /* 0x62 */
{ db,  decode_databyte },       /* 0x63 */
{ db,  decode_databyte },       /* 0x64 */
{ db,  decode_databyte },       /* 0x65 */
{ bit32,  NULL, DF_PREFIX },       /* 0x66 */
{ db,  decode_databyte },       /* 0x67 */
{ db,  decode_databyte },       /* 0x68 */
{ db,  decode_databyte },       /* 0x69 */
{ db,  decode_databyte },       /* 0x6a */
{ db,  decode_databyte },       /* 0x6b */
{ db,  decode_databyte },       /* 0x6c */
{ db,  decode_databyte },       /* 0x6d */
{ db,  decode_databyte },       /* 0x6e */
{ db,  decode_databyte },       /* 0x6f */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x70 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x71 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x72 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x73 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x74 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x75 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x76 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x77 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x78 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x79 */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7a */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7b */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7c */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7d */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7e */
{ jump,decode_cond_jump, DF_NOSPACE  }, /* 0x7f */
{ none,decode_bd8, 0, table_8x },       /* 0x80 */
{ none,decode_wd16,0, table_8x },       /* 0x81 */
{ db,  decode_databyte },               /* 0x82 */
{ none,decode_wd8, 0, table_8x },       /* 0x83 */
{ test,decode_br8 },            /* 0x84 */
{ test,decode_wr16 },           /* 0x85 */
{ xchg,decode_br8 },            /* 0x86 */
{ xchg,decode_wr16 },           /* 0x87 */
{ mov, decode_br8 },            /* 0x88 */
{ mov, decode_wr16 },           /* 0x89 */
{ mov, decode_r8b },            /* 0x8a */
{ mov, decode_r16w },           /* 0x8b */
{ mov, decode_ws },             /* 0x8c */
{ lea, decode_r16w },           /* 0x8d */
{ mov, decode_sw },             /* 0x8e */
{ pop, decode_w },              /* 0x8f */
{ nop },                        /* 0x90 */
{ xchg,decode_xchgax },         /* 0x91 */
{ xchg,decode_xchgax },         /* 0x92 */
{ xchg,decode_xchgax },         /* 0x93 */
{ xchg,decode_xchgax },         /* 0x94 */
{ xchg,decode_xchgax },         /* 0x95 */
{ xchg,decode_xchgax },         /* 0x96 */
{ xchg,decode_xchgax },         /* 0x97 */
{ cbw },                        /* 0x98 */
{ cwd },                        /* 0x99 */
{ call,decode_far },            /* 0x9a */
{ wait },                       /* 0x9b */
{ pushf },                      /* 0x9c */
{ popf },                       /* 0x9d */
{ sahf },                       /* 0x9e */
{ lahf },                       /* 0x9f */
{ mov, decode_almem },          /* 0xa0 */
{ mov, decode_axmem },          /* 0xa1 */
{ mov, decode_memal },          /* 0xa2 */
{ mov, decode_memax },          /* 0xa3 */
{ movs,decode_string, DF_NOSPACE },     /* 0xa4 */
{ movs,decode_string, DF_NOSPACE },     /* 0xa5 */
{ cmps,decode_string, DF_NOSPACE },     /* 0xa6 */
{ cmps,decode_string, DF_NOSPACE },     /* 0xa7 */
{ test,decode_ald8, },          /* 0xa8 */
{ test,decode_axd16 },          /* 0xa9 */
{ stos,decode_string, DF_NOSPACE },     /* 0xaa */
{ stos,decode_string, DF_NOSPACE },     /* 0xab */
{ lods,decode_string, DF_NOSPACE },     /* 0xac */
{ lods,decode_string, DF_NOSPACE },     /* 0xad */
{ scas,decode_string, DF_NOSPACE },     /* 0xae */
{ scas,decode_string, DF_NOSPACE },     /* 0xaf */
{ mov, decode_rd },             /* 0xb0 */
{ mov, decode_rd },             /* 0xb1 */
{ mov, decode_rd },             /* 0xb2 */
{ mov, decode_rd },             /* 0xb3 */
{ mov, decode_rd },             /* 0xb4 */
{ mov, decode_rd },             /* 0xb5 */
{ mov, decode_rd },             /* 0xb6 */
{ mov, decode_rd },             /* 0xb7 */
{ mov, decode_rd },             /* 0xb8 */
{ mov, decode_rd },             /* 0xb9 */
{ mov, decode_rd },             /* 0xba */
{ mov, decode_rd },             /* 0xbb */
{ mov, decode_rd },             /* 0xbc */
{ mov, decode_rd },             /* 0xbd */
{ mov, decode_rd },             /* 0xbe */
{ mov, decode_rd },             /* 0xbf */
{ db,  decode_databyte },       /* 0xc0 */
{ db,  decode_databyte },       /* 0xc1 */
{ ret, decode_d16 },            /* 0xc2 */
{ ret },                        /* 0xc3 */
{ les, decode_r16w },           /* 0xc4 */
{ lds, decode_r16w },           /* 0xc5 */
{ mov, decode_bd8 },            /* 0xc6 */
{ mov, decode_wd16 },           /* 0xc7 */
{ db,  decode_databyte },       /* 0xc8 */
{ db,  decode_databyte },       /* 0xc9 */
{ retf,decode_d16 },            /* 0xca */
{ retf },                       /* 0xcb */
{ intr,decode_int3 },           /* 0xcc */
{ intr,decode_d8 },             /* 0xcd */
{ into },                       /* 0xce */
{ iret },                       /* 0xcf */
{ none,decode_bbit1, 0, table_dx },     /* 0xd0 */
{ none,decode_wbit1, 0, table_dx },     /* 0xd1 */
{ none,decode_bbitcl, 0, table_dx },    /* 0xd2 */
{ none,decode_wbitcl, 0, table_dx },    /* 0xd3 */
{ aam, decode_adjust },         /* 0xd4 */
{ aad, decode_adjust },         /* 0xd5 */
{ db,  decode_databyte },       /* 0xd6 */
{ xlat },                       /* 0xd7 */
{ esc, decode_escape },         /* 0xd8 */
{ esc, decode_escape },         /* 0xd9 */
{ esc, decode_escape },         /* 0xda */
{ esc, decode_escape },         /* 0xdb */
{ esc, decode_escape },         /* 0xdc */
{ esc, decode_escape },         /* 0xdd */
{ esc, decode_escape },         /* 0xde */
{ esc, decode_escape },         /* 0xdf */
{ loopne,decode_disp },         /* 0xe0 */
{ loope,decode_disp },          /* 0xe1 */
{ loop,decode_disp },           /* 0xe2 */
{ jcxz,decode_disp },           /* 0xe3 */
{ in,  decode_ald8 },           /* 0xe4 */
{ in,  decode_axd8 },           /* 0xe5 */
{ out, decode_d8al },           /* 0xe6 */
{ out, decode_d8ax },           /* 0xe7 */
{ call,decode_disp16 },         /* 0xe8 */
{ jmp, decode_disp16 },         /* 0xe9 */
{ jmp, decode_far },            /* 0xea */
{ jmp, decode_disp },           /* 0xeb */
{ in,  decode_portdx },         /* 0xec */
{ in,  decode_portdx },         /* 0xed */
{ out, decode_portdx },         /* 0xee */
{ out, decode_portdx },         /* 0xef */
{ lock,NULL, DF_PREFIX },       /* 0xf0 */
{ none,decode_bioscall, DF_NOSPACE },  /* 0xf1 */
{ repnz,NULL, DF_PREFIX },      /* 0xf2 */
{ repz,NULL, DF_PREFIX },       /* 0xf3 */
{ hlt },                        /* 0xf4 */
{ cmc },                        /* 0xf5 */
{ none,decode_f6, 0, table_f67 }, /* 0xf6 */
{ none,decode_f7, 0, table_f67 }, /* 0xf7 */
{ clc },                        /* 0xf8 */
{ stc },                        /* 0xf9 */
{ cli },                        /* 0xfa */
{ sti },                        /* 0xfb */
{ cld },                        /* 0xfc */
{ std },                        /* 0xfd */
{ none,decode_b, 0, table_fe }, /* 0xfe */
{ none,decode_ff,0, table_ff }, /* 0xff */
};

#endif
