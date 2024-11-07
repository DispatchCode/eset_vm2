#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"

#define GET_IP(_vm)	\
	_vm->ip	\

#define INC_IP(_vm)	\
	++_vm->ip	\


#ifndef ESETVM2_DISASSEMBLY
struct esetvm2_instruction decode(struct esetvm2 *vm, int code_off);
#endif

void vm_print_internal_state(struct esetvm2 *vm)
{
	printf("\n\t[ ESETVM2 Internal State ]\n");
	printf("----------------------------\n");
	for(int i=0; i<16; i++) {
		printf("r%-2d: %4x (%d)\t", i, vm->regs[i], vm->regs[i]);
		if((i+1) % 4 == 0) printf("\n");
	}
	printf("\n----------------------------\n");
}

struct esetvm2 get_vm_instance(FILE * fp, int memory_size)
{
	struct esetvm2 eset_vm;

	eset_vm.ip = CODE_OFFSET;
	eset_vm.bit_shift = 0;
	eset_vm.memory = calloc(1, memory_size);
	eset_vm.memory_size = memory_size;
	memset(eset_vm.regs, 0, sizeof(eset_vm.regs));

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

#define REGS(_vm, _rindex) \
	_vm->regs[_rindex]	\

#define ARGS(_instr, _aindex)\
	_instr.arg[_aindex]

static void vm_execute(struct esetvm2 *vm, struct esetvm2_instruction instr)
{
	struct instr_info info;

	// Useful information that help decoding the instr.
	info = instr_table[instr.op_table_index];
 
	switch(instr.op_table_index) {
		 case 0:
		// TODO mov
		break;
		case 1: // loadConst
			REGS(vm, ARGS(instr, 0)) = instr.constant;
			printf("REG VALUE: %d, index_arg: %d\n", REGS(vm, ARGS(instr, 0)), ARGS(instr, 0));
		break;
		case 2: // add
			REGS(vm, ARGS(instr, 2)) = REGS(vm, ARGS(instr, 1)) + REGS(vm, ARGS(instr, 0));
		break;
	}

	
}

void execute(struct esetvm2 *vm)
{
	int code_off = 0;

	int instr_cnt = 3;
	while(1)
	{
		struct esetvm2_instruction instr = decode(vm, code_off);
		code_off += instr.len;

		vm_execute(vm, instr);
		
		//if(!instr.address)
			vm_shift_ptr(vm, instr.len);
		// TODO don't call the same function: dont sum but assign
		
		#ifdef VM_PRINT_STATE
			vm_print_internal_state(vm);
		#endif

		instr_cnt--;
		if(!instr_cnt)
			break;
	}

}
