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

/* This is MYTYPES.H  It contains definitions for the basic types to ease
   portability */

#ifndef MYTYPES_H
#define MYTYPES_H

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short int INT16;
typedef unsigned short int UINT16;
typedef signed int INT32;
typedef unsigned int UINT32;
#define PCEMU_LITTLE_ENDIAN

typedef UINT8 BYTE;
typedef UINT16 WORD;
typedef UINT32 DWORD;
typedef BYTE Boolean;

#endif
