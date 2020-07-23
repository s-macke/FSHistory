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

/* This is CPU.H  it contains definitions for cpu.c */

#ifndef CPU_H
#define CPU_H

#include "mytypes.h"

enum {
	AX = 0,
	CX,
	DX,
	BX,
	SP,
	BP,
	SI,
	DI
};

enum {
	AL = 0,
	CL,
	DL,
	BL,
	AH,
	CH,
	DH,
	BH
};

enum {
	SPL = 8,
	SPH,
	BPL,
	BPH,
	SIL,
	SIH,
	DIL,
	DIH
};

enum {
	ES = 0,
	CS,
	SS,
	DS
};


/* parameter x = result, y = source 1, z = source 2 */

#define SetCFB_Add(x, y) (CF = (BYTE)(x) < (BYTE)(y))
#define SetCFW_Add(x, y) (CF = (WORD)(x) < (WORD)(y))
#define SetCFB_Sub(y, z) (CF = (BYTE)(y) > (BYTE)(z))
#define SetCFW_Sub(y, z) (CF = (WORD)(y) > (WORD)(z))
#define SetZFB(x)    (ZF = !(BYTE)(x))
#define SetZFW(x)    (ZF = !(WORD)(x))
#define SetTF(x)    (TF = (x))
#define SetIF(x)    (IF = (x))
#define SetDF(x)    (DF = (x))
#define SetAF(x, y, z)    (AF = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetPF(x)        (PF = parity_table[(BYTE)(x)])
#define SetOFW_Add(x, y, z)   (OF = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x, y, z)   (OF = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x, y, z)   (OF = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x, y, z)   (OF = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)
#define SetSFW(x)       (SF = (x) & 0x8000)
#define SetSFB(x)       (SF = (x) & 0x80)


#define GetMemInc(Seg, Off) ((Seg)[(Off)++])

#define SegToMemPtr(Seg) (&memory[sregs[Seg] << 4])

#define GetMemB(Seg, Off) ((BYTE)(Seg)[(WORD)(Off)])

#define CompressFlags() (UINT32)(CF | (PF << 2) | (!(!AF) << 4) | (ZF << 6) \
                | (!(!SF) << 7) | (TF << 8) | (IF << 9) \
                | (DF << 10) | (!(!OF) << 11))

#define ExpandFlags(f) \
{ \
      CF = (f) & 1; \
      PF = ((f) & 4) == 4; \
      AF = (f) & 16; \
      ZF = ((f) & 64) == 64; \
      SF = (f) & 128; \
      TF = ((f) & 256) == 256; \
      IF = ((f) & 512) == 512; \
      DF = ((f) & 1024) == 1024; \
      OF = (f) & 2048; \
}


/* ChangeE(x) changes x to little endian from the machine's natural endian
    format and back again. Obviously there is nothing to do for little-endian
    machines... */

#if defined(PCEMU_LITTLE_ENDIAN)
#   define ChangeE(x) (WORD)(x)
//error
#else
#   define ChangeE(x) (WORD)(((x) << 8) | ((BYTE)((x) >> 8)))
#endif

#if defined(PCEMU_LITTLE_ENDIAN) && !defined(ALIGNED_ACCESS)
#   define ReadWord(x) (*(x))
#   define WriteWord(x, y) (*(x) = (y))
#   define CopyWord(x, y) (*x = *y)
#   define PutMemW(Seg, Off, x) (Seg[Off] = (BYTE)(x), Seg[(WORD)(Off)+1] = (BYTE)((x) >> 8))
#if 0
#   define PutMemW(Seg,Off,x) (*(WORD *)((Seg)+(WORD)(Off)) = (WORD)(x))
#endif
/* dtrg. `possible pointer alignment problem, Lint says */
#   define GetMemW(Seg, Off) ((WORD)Seg[Off] + ((WORD)Seg[(WORD)(Off)+1] << 8))
#if 0
#   define GetMemW(Seg,Off) (*(WORD *)((Seg)+(Off))) */
#endif
#else
#   define ReadWord(x) ((WORD)(*((BYTE *)(x))) + ((WORD)(*((BYTE *)(x)+1)) << 8))
#   define WriteWord(x,y) (*(BYTE *)(x) = (BYTE)(y), *((BYTE *)(x)+1) = (BYTE)((y) >> 8))
#   define CopyWord(x,y) (*(BYTE *)(x) = *(BYTE *)(y), *((BYTE *)(x)+1) = *((BYTE *)(y)+1))
#   define PutMemW(Seg,Off,x) (Seg[Off] = (BYTE)(x), Seg[(WORD)(Off)+1] = (BYTE)((x) >> 8))
#   define GetMemW(Seg,Off) ((WORD)Seg[Off] + ((WORD)Seg[(WORD)(Off)+1] << 8))
#endif


#endif
