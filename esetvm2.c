#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"


struct esetvm2 *vm;

#define SET_IP(new_ip)	\
	vm_th->ip = new_ip

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
	(unlikely(instr.mem_bytes[_index]) ? vm_read_mem(vm_th, instr, _index) : (int64_t)REGS(ARGS(_index))) 

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

static void init_vm_instance(FILE * fp, int file_size)
{
	vm = calloc(1, sizeof(struct esetvm2));
	
	vm->thread_count = 1;	
	vm->thread_state = calloc(10, sizeof(struct vm_thread));
	vm->thread_state[0].ip = 0;

	vm->threads = calloc(10, sizeof(pthread_t));
}

static void load_into_memory(struct esetvm2hdr *vm_hdr, uint8_t *buff) 
{
	vm->code = calloc(vm_hdr->code_size, sizeof(uint8_t));
	vm->data = calloc(vm_hdr->data_size, sizeof(uint8_t));

	int init_data_off = vm_hdr->code_size + CODE_OFFSET;
	memcpy(vm->code, buff + CODE_OFFSET, vm_hdr->code_size);
	if(vm_hdr->initial_data_size)
		memcpy(vm->data, buff + init_data_off, vm_hdr->initial_data_size);
}

struct esetvm2hdr * vm_init(FILE *fp, int file_size, char *image_name)
{
	struct esetvm2hdr *vm_hdr;	
	uint8_t *buff = calloc(1, file_size);

	fread(buff, sizeof(uint8_t), file_size, fp);
	vm_hdr = vm_load_hdr(buff);	

	init_vm_instance(fp, file_size);
	load_into_memory(vm_hdr, buff);

	char file_path[100] = "samples/";
	strncat(file_path, image_name, sizeof(image_name));
	strncat(file_path, ".bin", 5);

	vm->hbin = fopen(file_path, "rwb");
	
	free(buff);
	
	return vm_hdr;
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

static inline void vm_call(struct vm_thread *vm_th, struct esetvm2_instruction instr) {
	if(unlikely(vm_th->tos == VM_STACK_SIZE)) {
		printf("WARNING: thread %d reach the maximum TOS. Expected crash due to Stack Overflow\n");
	}

	vm_th->call_stack[vm_th->tos++] = vm_th->ip + instr.len;
}

static inline uint32_t vm_ret(struct vm_thread *vm_th) {
	if(unlikely(vm_th->tos == 0)) {
		printf("WARNING: thread %d no more elements in the stack\n");
	}

	return vm_th->call_stack[--vm_th->tos];
}

static inline int64_t vm_read_mem(struct vm_thread *vm_th, struct esetvm2_instruction instr, int index) {
	int bytes = instr.mem_bytes[index] - 1;
	int64_t val = vm->data[REGS(ARGS(index)) + bytes];
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
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 1: // loadConst
			WARGX(instr.constant);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 2: // add
			val = MATH_OP(+);
			WARGX(val);	
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 3: // sub
			val = MATH_OP(-);
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 4: // div
			val = MATH_OP(/);
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 5: // mod
			val = MATH_OP(%);
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 6: // mul
			val = MATH_OP(*);
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 7: {// compare
			int64_t a0 = RARGX(0);
			int64_t a1 = RARGX(1);
			val = a0 > a1;
			if(a0 < a1)
				val = -1;
			
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		};
		break;
		case 8: // jump
			SET_IP(instr.address);
		break;
		case 9: // jumpEqual
			if(RARGX(0) == RARGX(1)) {
				SET_IP(instr.address);
			} else {
				vm_shift_ptr(vm_th, instr.len);
			}
		break;
		case 10: // read
			// TODO handle binary file name
			uint64_t a1 = RARGX(0); // offset in input file
			uint64_t a2 = RARGX(1); // number of bytes to read
			uint64_t a3 = RARGX(2); // mem. addr. to which bytes will be stored

			fseek(vm->hbin, a1, SEEK_SET);

			uint64_t curr_byte = ftell(vm->hbin);
			fread(&vm->data[a3], sizeof(uint8_t), a2, vm->hbin);
			
			// a2 can-t be trusted: bytes read could be less if not enough data exists
			curr_byte = ftell(vm->hbin) - curr_byte;
			WARGX(curr_byte);

			vm_shift_ptr(vm_th, instr.len);
		break;
		//case 11: // write
		//break;
		case 12: // consoleRead
			scanf("%lx", &val);
			WARGX(val);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 13: // consoleWrite
			printf("%016lx\n", RARGX(0));
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 14: // createThread
			vm_setup_new_thread(vm_th, instr, instr.address);	
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 15: // joinThread
			vm_wait(vm_th, instr);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 16: // hlt
			vm_th->active = 0;
			vm_shift_ptr(vm_th, instr.len);
			pthread_cond_signal(&vm->thread_state[vm_th->index].cond_active);
		break;
		case 17: // sleep
			int ms = REGS(arg) / 1000;
			sleep(ms);
			vm_shift_ptr(vm_th, instr.len);
		break;
		case 18: // call
			vm_call(vm_th, instr);
			SET_IP(instr.address);
		break;
		case 19: // ret
			SET_IP(vm_ret(vm_th));
		break;
		default:
			printf("\nWARNING: invalid opcode 0x%x. Stopping thread %d...\n", opcode_map[instr.op_table_index], vm_th->index);
			vm_th->active = 0; 
			vm_shift_ptr(vm_th, instr.len);
			pthread_cond_signal(&vm->thread_state[vm_th->index].cond_active);
		break;
	}
}

#ifndef ESETVM2_DISASSEMBLY

void *vm_thread_run(struct vm_thread *vm_th)
{
	while(vm_th->active)
	{
		struct esetvm2_instruction instr = decode(vm_th);

		vm_execute(vm_th, instr);
		
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
	vm->thread_state[thread_count].ip = address;
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

	vm->thread_state[0].call_stack = calloc(VM_STACK_SIZE, sizeof(uint32_t)); // TODO 4-KB for now, but make it dynamic
	vm->thread_state[0].tos = 0; // Stack starts from 0
	
	pthread_create(&vm->threads[0], NULL, (void*)vm_thread_run, (void*)&vm->thread_state[0]);
	pthread_detach(vm->threads[0]);

	vm_wait_on_active(&vm->thread_state[0], 0);
}
#endif
