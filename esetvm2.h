#ifndef ESETVM2_HEADER
#define ESETVM2_HEADER

#include <stdio.h>
#include <stdint.h>

#include "config.h"

#include "esetvm2hdr.h"
#include "esetvm2decode.h"

// whole VM memory
//static uint8_t *memory;
//static int memory_size;

struct vm_thread
{
	// index inside threads
	int index;

	// 0 - inactive; 1 - active
	int active;

	// VM registers
	int64_t regs[16];
	// pointer to the next instr.
	uint32_t ip;

	// thread stack
	uint8_t *call_stack;

};

struct esetvm2
{
	int thread_count;
	// thread_count is the actual nr of elements
	struct vm_thread *thread_state;
};

struct esetvm2 get_vm_instance(FILE *fp, int );
struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int );
struct esetvm2hdr *load_task(struct esetvm2 *);

/* interface over VM and memory */
uint8_t vm_mem_ru8(struct vm_thread *);
uint8_t vm_next_op(struct vm_thread *);
void vm_mem_wu8(struct vm_thread *, uint8_t val);
void vm_shift_ptr(struct vm_thread *vm, uint8_t bits);
int vm_end_of_code(struct vm_thread *vm);
void vm_start(struct esetvm2 *vm);

#endif
