#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"


static int th_cnt = 0;
 pthread_t *threads;

uint8_t *memory;
int memory_size;

#define GET_IP(_vm)	\
	_vm->ip	\

#define INC_IP(_vm)	\
	++_vm->ip	\


#ifndef ESETVM2_DISASSEMBLY
struct esetvm2_instruction decode(struct vm_thread *vm_th);
#endif

void vm_print_internal_state(struct vm_thread *vm_th)
{
	printf("\n\t[ ESETVM2 Internal State ]\n");
	printf("----------------------------\n");
	for(int i=0; i<16; i++) {
		printf("r%-2d: %4x (%d)\t", i, vm_th->regs[i], vm_th->regs[i]);
		if((i+1) % 4 == 0) printf("\n");
	}
	printf("\n----------------------------\n");
}

struct esetvm2 get_vm_instance(FILE * fp, int file_size)
{
	struct esetvm2 eset_vm;
	
	memory = calloc(1, file_size);
	memory_size = file_size;
	eset_vm.thread_count = 1;	
	eset_vm.thread_state = calloc(1, sizeof(struct vm_thread));
	eset_vm.thread_state[0].ip = CODE_OFFSET_BIT;

	return eset_vm;
}

struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int memory_size)
{
	fread(memory, sizeof(uint8_t), memory_size, fp);

	return load_task(eset_vm);
}

static inline uint8_t vm_mem_ru8n(struct vm_thread *vm_th, int bytes)
{
	return memory[GET_IP(vm_th) + bytes];
}

inline uint8_t vm_mem_ru8(struct vm_thread *vm_th)
{
	return vm_mem_ru8n(vm_th, 0);
}

inline uint8_t vm_next_op(struct vm_thread *vm_th)
{
	uint8_t tmp_op;
	
	// read 8bits 
	for(int i=0; i<8; i++) {
		tmp_op = (tmp_op << 1) | (get_bit_at(vm_th->ip + i));
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

inline void vm_shift_ptr(struct vm_thread *vm_th, uint8_t bits)
{
	vm_th->ip += bits;
}

inline int vm_end_of_code(struct vm_thread *vm_th)
{
	return vm_th->ip < memory_size;
}

#define REGS(_vm, _rindex) \
	_vm->regs[_rindex]	\

#define ARGS(_instr, _aindex)\
	_instr.arg[_aindex]

#define MATH_OP(_vm, _instr, _sign)	\
	REGS(_vm, ARGS(_instr, 2)) = REGS(_vm, ARGS(_instr, 0)) _sign REGS(_vm, ARGS(_instr, 1))

static void vm_execute(struct vm_thread *vm_th, struct esetvm2_instruction instr)
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
			REGS(vm_th, ARGS(instr, 0)) = instr.constant;
		break;
		case 2: // add
			MATH_OP(vm_th, instr, +);
		break;
		case 3: // sub
			MATH_OP(vm_th, instr, -);
		break;
		case 4: // div
			MATH_OP(vm_th, instr, /);
		break;
		case 5: // mod
			MATH_OP(vm_th, instr, %);
		break;
		case 6: // mul
			MATH_OP(vm_th, instr, *);
		break;
		case 7: { // compare
			REGS(vm_th, ARGS(instr, 2)) = REGS(vm_th, ARGS(instr, 0)) > REGS(vm_th, ARGS(instr, 1));

			if(REGS(vm_th, ARGS(instr, 0)) < REGS(vm_th, ARGS(instr, 1)))
				REGS(vm_th, ARGS(instr, 2)) = -1;
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
			printf("%016x\n", REGS(vm_th, ARGS(instr, 0)));
		break;
		case 14: // createThread
		break;
		case 15: // joinThread
		break;
		case 16: // hlt
			vm_th->active = 0;						
		break;
	}

	
}

void *vm_thread_run(struct vm_thread *vm_th)
{
	while(vm_th->active)
	{
		struct esetvm2_instruction instr = decode(vm_th);

		vm_execute(vm_th, instr);
		
		// Move the increment inside the execution or better the decode
		//if(!instr.address)
			vm_shift_ptr(vm_th, instr.len);
		// TODO don't call the same function: dont sum but assign
	
		#ifdef VM_PRINT_STATE
			vm_print_internal_state(vm_th);
		#endif
	}
}

void vm_setup_new_thread(struct esetvm2 *vm, int thread_nr, uint32_t address) {
// TODO copy registers from actual thread
// set IP to the "address" 
}

#ifndef ESETVM2_DISASSEMBLY
void vm_start(struct esetvm2 *vm)
{
	// initial thread
	threads = calloc(1, sizeof(pthread_t));
	vm->thread_state[0].index  = 0;
	vm->thread_state[0].active = 1;
	
	pthread_create(&threads[0], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[0]);

	pthread_join(threads[0], NULL);

}
#endif
