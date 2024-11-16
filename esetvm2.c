#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"


struct esetvm2 *vm;

#define GET_IP()		\
	vm_th->ip			\

#define INC_IP()		\
	++vm_th->ip			\

#define REGS(_rindex)	\
	vm_th->regs[_rindex]

#define ARGS(_aindex)	\
	instr.arg[_aindex]

/* Read from source, eiter memory or register  */
#define RARGX(_index)																				\
	(unlikely(instr.mem_bytes[_index]) ? vm_read_mem(vm_th, instr, _index) : REGS(ARGS(_index))) 

/* Write to the destination, either memory or register  */
#define WARGX(_val)																		\
{																						\
	if(instr.mem_bytes[dst_index])														\
		memcpy(&vm->data[REGS(ARGS(dst_index))], &_val, instr.mem_bytes[dst_index]);	\
	else																				\
		REGS(arg) = _val;																\
};

/* Math operations, between registers or memory  */
#define MATH_OP(math_op)			\
	(RARGX(0) math_op RARGX(1))


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
	vm->memory = calloc(1, file_size);
	vm->memory_size = file_size;

	vm->thread_count = 1;	
	vm->thread_state = calloc(10, sizeof(struct vm_thread));
	vm->thread_state[0].ip = CODE_OFFSET_BIT;
	
	vm->threads = calloc(10, sizeof(pthread_t));
}

struct esetvm2hdr * vm_load_task(FILE *fp, int memory_size)
{
	struct esetvm2hdr *vm_hdr;	

	fread(vm->memory, sizeof(uint8_t), memory_size, fp);
	vm_hdr = load_task();	

	vm->data = calloc(vm_hdr->data_size, sizeof(uint8_t));

	return vm_hdr;
}

static inline uint8_t vm_mem_ru8n(struct vm_thread *vm_th, int bytes)
{
	return vm->memory[GET_IP() + bytes];
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
	return vm_th->ip < vm->memory_size;
}


static inline uint64_t vm_read_mem(struct vm_thread *vm_th, struct esetvm2_instruction instr, int index) {
	int bytes = instr.mem_bytes[index] - 1;
	uint64_t val = vm->data[REGS(ARGS(index)) + bytes];
	int reg_index = REGS(ARGS(index));

	while(bytes-- > 0)
		val = (val<<8) | vm->data[reg_index + bytes];
	
	return val;
}

static void vm_execute(struct vm_thread *vm_th, struct esetvm2_instruction instr)
{
	int64_t val;
	struct instr_info info;
	uint8_t arg;
	int dst_index;

	info = instr_table[instr.op_table_index];

	dst_index = info.nr_args-1;
	arg = ARGS(dst_index);
	
	switch(instr.op_table_index) {
		case 0: // mov
			val = RARGX(0);
			WARGX(val)
		break;
		case 1: // loadConst
			WARGX(instr.constant)
		break;
		case 2: // add
			val = MATH_OP(+);
			WARGX(val)	
		break;
		case 3: // sub
			val = MATH_OP(-);
			WARGX(val)
		break;
		case 4: // div
			val = MATH_OP(/);
			WARGX(val);
		break;
		case 5: // mod
			val = MATH_OP(%);
			WARGX(val);
		break;
		case 6: // mul
			val = MATH_OP(*);
			WARGX(val);
		break;
		case 7: {// compare
			val = RARGX(0) > RARGX(1);
			if(RARGX(0) < RARGX(1))
				val = -1;
			
			WARGX(val)
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
			printf("%016lx\n", RARGX(0));
		break;
		case 14: // createThread
			vm_setup_new_thread(vm_th, instr, instr.address);	
		break;
		case 15: // joinThread
			vm_wait(vm_th, instr);
		break;
		case 16: // hlt
			//printf("hlt thread: %d\n", vm_th->index);
			vm_th->active = 0;
			pthread_cond_signal(&vm->thread_state[vm_th->index].cond_active);
		break;
		case 17: // sleep
			int ms = REGS(arg) / 1000;
			sleep(ms);
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

inline void vm_wait_on_active(struct vm_thread *vm_th, int th_index)
{
	pthread_mutex_lock(&vm->thread_state[th_index].lock_active);
	while(vm->thread_state[th_index].active) {
		pthread_cond_wait(&vm->thread_state[th_index].cond_active, &vm->thread_state[th_index].lock_active);
	}
	pthread_mutex_unlock(&vm->thread_state[th_index].lock_active);

}

void vm_wait(struct vm_thread *vm_th, struct esetvm2_instruction instr)
{
	int th_index = REGS(ARGS(0));
	vm_wait_on_active(vm_th, th_index);
} 

void vm_setup_new_thread(struct vm_thread *vm_th, struct esetvm2_instruction instr, uint32_t address) {
	// TODO protect with mutex thread_count
	int thread_count = vm->thread_count;

	//printf("setup new thread (thread_count: %d)\n", thread_count);
	
	REGS(ARGS(0)) = thread_count;

	//vm->thread_state = realloc(vm->thread_state, (1+thread_count)*sizeof(struct vm_thread));
	vm->thread_state[thread_count].index = thread_count;
	vm->thread_state[thread_count].active = 1;
	vm->thread_state[thread_count].ip = address + CODE_OFFSET_BIT;
	memcpy(&vm->thread_state[thread_count].regs, &vm_th->regs, 16*(sizeof(int64_t)));	

	//threads = realloc(threads, (thread_count+1)*sizeof(pthread_t));

	pthread_create(&vm->threads[thread_count], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[thread_count]);
	pthread_detach(vm->threads[thread_count]);
	vm->thread_count++;
}

void vm_start()
{
	// initial thread
	vm->thread_state[0].index  = 0;
	vm->thread_state[0].active = 1;
	
	pthread_create(&vm->threads[0], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[0]);
	pthread_detach(vm->threads[0]);

	vm_wait_on_active(&vm->thread_state[0], 0);
}
#endif
