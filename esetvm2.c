#include <stdio.h>
#include <malloc.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"

#define GET_IP(_vm)	\
	_vm->ip	\

#define INC_IP(_vm)	\
	++_vm->ip	\


struct esetvm2 get_vm_instance(FILE * fp, int memory_size)
{
	struct esetvm2 eset_vm;

	eset_vm.ip = CODE_OFFSET;
	eset_vm.bit_shift = 0;
	eset_vm.memory = malloc(memory_size);
	eset_vm.memory_size = memory_size;

	return eset_vm;
}

struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int memory_size)
{
	fread(eset_vm->memory, sizeof(uint8_t), memory_size, fp);

	return load_task(eset_vm);
}

static inline uint8_t vm_mem_ru8n(struct esetvm2 *vm, int bytes)
{
	return vm->memory[GET_IP(vm) + bytes];
}

inline uint8_t vm_mem_ru8(struct esetvm2 *vm)
{
	return vm_mem_ru8n(vm, 0);
}

inline uint8_t vm_next_op(struct esetvm2 *vm)
{
	switch(vm->bit_shift) {
		case 0: case 1: case 2: case 3:
			return vm_mem_ru8(vm) << vm->bit_shift;
		case 4: case 5: case 6: case 7:
			return (vm_mem_ru8(vm) << vm->bit_shift) | (vm_mem_ru8n(vm,1) >> (8 - vm->bit_shift));
		default:
			INC_IP(vm);
			vm->bit_shift %= 8;
			return vm_mem_ru8(vm) << vm->bit_shift;
	}
}

inline void vm_shift_ptr(struct esetvm2 *vm, int bits)
{
	vm->ip 		  += (bits >> 3);
	vm->bit_shift += (bits % 8);
}

inline int vm_end_of_code(struct esetvm2 *vm)
{
	return vm->ip < vm->memory_size;
}
