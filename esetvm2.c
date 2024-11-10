#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"


static int th_cnt = 0;
 pthread_t *threads;

uint8_t *memory;
int memory_size;
struct esetvm2 *vm;

#define GET_IP(_vm)	\
	_vm->ip	\

#define INC_IP(_vm)	\
	++_vm->ip	\


#ifndef ESETVM2_DISASSEMBLY
struct esetvm2_instruction decode(struct vm_thread *vm_th);
#endif

void vm_print_internal_state(struct vm_thread *vm_th)
{
	printf("\n\t[ ESETVM2 Internal State (Thread: %d) ]\n", vm_th->index);
	printf("----------------------------\n");
	for(int i=0; i<16; i++) {
		printf("r%-2d: %4lx (%d)\t", i, vm_th->regs[i], vm_th->regs[i]);
		if((i+1) % 4 == 0) printf("\n");
	}
	printf("\n----------------------------\n");
}

void init_vm_instance(FILE * fp, int file_size)
{
	vm = calloc(1, sizeof(struct esetvm2));
	memory = calloc(1, file_size);
	memory_size = file_size;

	vm->thread_count = 1;	
	vm->thread_state = calloc(10, sizeof(struct vm_thread));
	vm->thread_state[0].ip = CODE_OFFSET_BIT;
}

struct esetvm2hdr * vm_load_task(FILE *fp, int memory_size)
{
	struct esetvm2hdr *vm_hdr;	

	fread(memory, sizeof(uint8_t), memory_size, fp);
	vm_hdr = load_task();	

	vm->data = calloc(vm_hdr->data_size, sizeof(uint8_t));

	return vm_hdr;
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

// TODO handle reg / mem operation checking reg_or_mem[]
#define REGS(_vm, _rindex) \
	_vm->regs[_rindex]	\

#define ARGS(_instr, _aindex)\
	_instr.arg[_aindex]

// TODO support also memory operations 
#define MATH_OP(_vm, _instr, _sign)	\
	REGS(_vm, ARGS(_instr, 2)) = REGS(_vm, ARGS(_instr, 0)) _sign REGS(_vm, ARGS(_instr, 1))

static void vm_execute(struct vm_thread *vm_th, struct esetvm2_instruction instr)
{
	//printf("vm_execute, thread: %d ", vm_th->index);
	struct instr_info info;

	// Useful information that help decoding the instr.
	info = instr_table[instr.op_table_index];
 	//printf("%s\n", info.mnemonic);
	//printf("INSTR_TABLE_INDEX: %d\n", instr.op_table_index);
	
	switch(instr.op_table_index) {
		case 0: // mov
			uint64_t mem_index = REGS(vm_th, ARGS(instr, 1));
			//printf("mem_index, mov: %d ", mem_index);
 
			uint64_t val = REGS(vm_th, ARGS(instr,0));
			//printf("mov, val: %d, byte to move: %d\n", val, instr.reg_or_mem[1]);
			
			memcpy(&vm->data[mem_index], &val, instr.reg_or_mem[1]);
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
		{
			if(instr.ss[0]) {
				uint64_t val;
				memcpy(&val, &vm->data[REGS(vm_th, ARGS(instr,0))], instr.reg_or_mem[0]);
				printf("%016lx\n", val);
			} else
				printf("%016lx\n", REGS(vm_th, ARGS(instr, 0)));
		};
		break;
		case 14: // createThread
			uint32_t address = instr.address;
			vm_setup_new_thread(vm_th, instr, address);	
		break;
		case 15: // joinThread
			vm_wait(vm_th, instr);
		break;
		case 16: // hlt
			//printf("hlt thread: %d\n", vm_th->index);
			vm_th->active = 0;
			vm_print_internal_state(vm_th);
			//printf("DATA from thread %d: %x\n", vm_th->index, vm->data[0]); 						
		break;
	}
}

#ifndef ESETVM2_DISASSEMBLY

void *vm_thread_run(struct vm_thread *vm_th)
{
	//printf("vm_thread_run, thread: %d\n", vm_th->index);
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
		
		// let other threads run
		sleep(0.1);
	}
}

void vm_wait(struct vm_thread *vm_th, struct esetvm2_instruction instr)
{
	int th_index = REGS(vm_th, ARGS(instr, 0));
	// TODO use pthread_cond
	while(vm->thread_state[th_index].active) {
		sleep(0.1);
	}
} 

void vm_setup_new_thread(struct vm_thread *vm_th, struct esetvm2_instruction instr, uint32_t address) {
	// TODO protect with mutex thread_count
	int thread_count = vm->thread_count;

	//printf("setup new thread (thread_count: %d)\n", thread_count);
	
	REGS(vm_th, ARGS(instr, 0)) = thread_count;

	//vm->thread_state = realloc(vm->thread_state, (1+thread_count)*sizeof(struct vm_thread));
	vm->thread_state[thread_count].index = thread_count;
	vm->thread_state[thread_count].active = 1;
	vm->thread_state[thread_count].ip = address + CODE_OFFSET_BIT;
	memcpy(&vm->thread_state[thread_count].regs, &vm_th->regs, 16*(sizeof(int64_t)));	

	//threads = realloc(threads, (thread_count+1)*sizeof(pthread_t));

	pthread_create(&threads[thread_count], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[thread_count]);
	pthread_detach(threads[thread_count]);
	vm->thread_count++;
}

void vm_start()
{
	// initial thread
	threads = calloc(10, sizeof(pthread_t));
	vm->thread_state[0].index  = 0;
	vm->thread_state[0].active = 1;
	
	pthread_create(&threads[0], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[0]);
	pthread_detach(threads[0]);

	while(vm->thread_state[0].active) sleep(0.1);

}
#endif
