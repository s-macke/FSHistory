#ifndef ALLOC_H
#define ALLOC_H

#include <stdbool.h>
#include <stdint.h>

uint16_t Allocate(uint16_t paragraphs);

void Free(uint16_t seg);

extern uint16_t nextfreeseg;

bool Modify(uint16_t seg, uint16_t paragraphs);

uint16_t AvailableRam();

void AllocInit();

#endif
