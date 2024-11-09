#ifndef COMMON_H
#define COMMON_H

#include "esetvm2.h"

extern uint8_t *memory;

static inline int get_bit_at(int pos)
{
	int byte_index = pos >> 3;
	int bit_index  = pos % 8;
	return (memory[byte_index] >> (7 - bit_index) & 1);
}

static inline int64_t read_const(int start, int num_bits) {
	int64_t constant = 0;
	for(int i=0; i < num_bits; i++) {
		constant = (constant << 1) | get_bit_at(start + (num_bits - 1) - i);
	}
	return constant;
}

#endif
