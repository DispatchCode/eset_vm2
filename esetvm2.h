#ifndef ESETVM2_HEADER
#define ESETVM2_HEADER

#include <stdio.h>
#include <stdint.h>

#include "esetvm2hdr.h"
#include "esetvm2decode.h"

struct esetvm2
{
	// VM registers
	int64_t regs[15];
	// pointer to the next instr.
	uint32_t ip;
	// whole VM memory
	uint8_t *memory;
};

struct esetvm2 get_vm_instance(FILE *fp, int memory_size);
struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int memory_size);
struct esetvm2hdr *load_task(struct esetvm2 *);

#endif
