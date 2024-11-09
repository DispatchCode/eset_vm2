#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"

#define GET_IP(_vm)	\
	_vm->ip	\

#define INC_IP(_vm)	\
	++_vm->ip	\


#ifndef ESETVM2_DISASSEMBLY
struct esetvm2_instruction decode(struct esetvm2 *vm);
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

	eset_vm.ip = CODE_OFFSET_BIT;
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
	uint8_t tmp_op;
	
	// read 8bits 
	for(int i=0; i<8; i++) {
		tmp_op = (tmp_op << 1) | (get_bit_at(vm, vm->ip + i));
	}
	// max number of bits is 6, so clear first 2 bits
	tmp_op ^= 3;

	// get the "group" from the opcode
	uint8_t grp = (tmp_op & 0xE0) >> 5;
	// mask of the group (used to retrieve the index)
	uint8_t grp_mask = op_grp_table[grp];
	// the "index" part of the opcode
	uint8_t grp_index = (tmp_op & grp_mask) >> op_index_shift[grp];
	// byte aligned opcode, like in the opcode_map
	uint8_t op = (grp << 5) | (grp_index << op_index_shift[grp]);

	return op;
}

inline void vm_shift_ptr(struct esetvm2 *vm, uint8_t bits)
{
	vm->ip += bits;
}

inline int vm_end_of_code(struct esetvm2 *vm)
{
	return vm->ip < vm->memory_size;
}

#define REGS(_vm, _rindex) \
	_vm->regs[_rindex]	\

#define ARGS(_instr, _aindex)\
	_instr.arg[_aindex]

#define MATH_OP(_vm, _instr, _sign)	\
	REGS(_vm, ARGS(_instr, 2)) = REGS(_vm, ARGS(_instr, 0)) _sign REGS(_vm, ARGS(_instr, 1))

static void vm_execute(struct esetvm2 *vm, struct esetvm2_instruction instr)
{
	struct instr_info info;

	// Useful information that help decoding the instr.
	info = instr_table[instr.op_table_index];
 
	//printf("INSTR_TABLE_INDEX: %d\n", instr.op_table_index);
	
	switch(instr.op_table_index) {
		 case 0:
		// TODO mov
		break;
		case 1: // loadConst
			REGS(vm, ARGS(instr, 0)) = instr.constant;
		break;
		case 2: // add
			MATH_OP(vm, instr, +);
		break;
		case 3: // sub
			MATH_OP(vm, instr, -);
		break;
		case 4: // div
			MATH_OP(vm, instr, /);
		break;
		case 5: // mod
			MATH_OP(vm, instr, %);
		break;
		case 6: // mul
			MATH_OP(vm, instr, *);
		break;
		case 7: { // compare
			REGS(vm, ARGS(instr, 2)) = REGS(vm, ARGS(instr, 0)) > REGS(vm, ARGS(instr, 1));

			if(REGS(vm, ARGS(instr, 0)) < REGS(vm, ARGS(instr, 1)))
				REGS(vm, ARGS(instr, 2)) = -1;
		};
		break;
		case 8: // jump
		break;
		case 9: // jumpEqual
		break;
		case 10: // read
		break;
		case 11: // write
		break;
		case 12: // consoleRead
		break;
		case 13: // consoleWrite
			printf("%016x\n", REGS(vm, ARGS(instr, 0)));
		break;
		case 14: // createThread
		break;
		case 15: // joinThread
		break;
		case 16: // hlt
			
		break;
	}

	
}

#ifndef ESETVM2_DISASSEMBLY
void execute(struct esetvm2 *vm)
{

	int instr_cnt = 14;
	while(1)
	{
		struct esetvm2_instruction instr = decode(vm);

		vm_execute(vm, instr);
		
// Move the increment inside the execution or better the decode
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
#endif
