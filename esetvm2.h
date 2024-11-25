#ifndef ESETVM2_HEADER
#define ESETVM2_HEADER

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "config.h"

#include "esetvm2hdr.h"
#include "esetvm2decode.h"

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

// 4-KB stack
#define VM_STACK_SIZE 1024 

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
	uint32_t *call_stack;
	int tos;

	// wait for a mutex
	int wait;

	pthread_mutex_t lock_active;
	pthread_cond_t cond_active;

	pthread_mutex_t lock_wait;
	pthread_cond_t cond_wait;
};

struct esetvm2_lock {
	int64_t lock_obj;
	int locked;
	int threads[10];
	int tos;

	pthread_mutex_t mutex_locked;
	pthread_mutex_t mutex_threads;
};

/* 
 * TODO hashmap to store locks: lock object is the key,
 *		while the value is the thread
 *
 *	-	If the lock is NOT present in the table, then add it
 * 		with thread_index value
 *	-	if IT IS present, then check the value:
 *		- if IT HAS the same value, message on screen (UB, not possibile, should never happen)
 *		- if HAS NOT the same value, wait on it, till removed from the hashmap by the other thread
 */

struct esetvm2
{
	// nr of threads
	int thread_count;
	// vm thread: registers and stack
	struct vm_thread *thread_state;
	// real thread
	pthread_t *threads;
	
	uint8_t *code;
	
	// "data" size buffer (memory used by the program)
	uint8_t *data;
	
	// handle to a binary file
	FILE *hbin;

	struct esetvm2_lock *locks;
	int lock_tos;
};

struct esetvm2hdr * vm_init(FILE *fp, int, char*);
struct esetvm2hdr * vm_load_hdr(uint8_t *);

/* interface over VM and memory */
uint8_t vm_mem_ru8(struct vm_thread *);
uint8_t vm_next_op(struct vm_thread *);
void vm_mem_wu8(struct vm_thread *, uint8_t val);
void vm_shift_ptr(struct vm_thread *vm, uint8_t bits);
int vm_end_of_code(struct vm_thread *vm);
void vm_start();
void vm_wait(struct vm_thread *vm_th, struct esetvm2_instruction instr);
void vm_setup_new_thread(struct vm_thread *vm_th, struct esetvm2_instruction instr, uint32_t address);
#endif
