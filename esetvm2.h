#ifndef ESETVM2_HEADER
#define ESETVM2_HEADER

#include <stdio.h>
#include <stdint.h>

#include "esetvm2hdr.h"
#include "esetvm2decode.h"

struct esetvm2
{
	// VM registers
	int64_t regs[16];
	// pointer to the next instr.
	uint32_t ip;
	// whole VM memory
	uint8_t *memory;
	// used to maintain bit alignment
	int bit_shift;

	int memory_size;
};

struct esetvm2 get_vm_instance(FILE *fp, int memory_size);
struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int memory_size);
struct esetvm2hdr *load_task(struct esetvm2 *);

/* interface over VM and memory */
uint8_t vm_mem_ru8(struct esetvm2 *);
uint8_t vm_next_op(struct esetvm2 *);
void vm_mem_wu8(struct esetvm2 *, uint8_t val);
void vm_shift_ptr(struct esetvm2 *vm, int bits);
int vm_end_of_code(struct esetvm2 *vm);

#endif
