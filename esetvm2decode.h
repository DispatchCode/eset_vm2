#ifndef ESETVM2DECODE_HEADER
#define ESETVM2DECODE_HEADER

#include <stdint.h>

#define MOV 		0x00
#define LOAD_CONST 	0x20
#define ADD 		0x44
#define SUB 		0x48
#define DIV	 		0x4C
#define MOD 		0x50
#define MUL 		0x54
#define COMPARE 	0x60
#define JUMP		0x64
#define JUMP_EQUAL 	0x70
#define READ		0x80
#define WRITE		0x88
#define CONS_READ	0x90
#define CONS_WRITE	0x98
#define C_THREAD	0xA0
#define J_THREAD	0xA8
#define HLT			0xB0
#define SLEEP		0xB8
#define CALL		0xC0
#define RET			0xD0
#define LOCK		0xE0
#define UNLOCK		0xF0

uint8_t opcode_map[23] = {
	MOV, LOAD_CONST, ADD, SUB, DIV, MOD, MUL, COMPARE, JUMP, 
	JUMP_EQUAL, READ_WRITE, CONS_READ, CONS_WRITE, C_THREAD,
	J_THREAD, HLT, SLEEP, CALL, RET, LOCK, UNLOCK	
};

struct instr_info
{
	uint8_t op_size;
	uint8_t len;
	uint8_t nr_args;
	uint8_t addr;
	uint8_t constant;
};

#define INIT_INSTR_PROP(_op_size, _len, _nr_args, _addr, _constant)	\
		{															\
			.op_size   = _op_size,									\
			.len	   = _len, 										\
			.nr_args   = _nr_args,									\
			.addr	   = _addr,										\
			.constant  = _constant									\
		}															\

struct esetvm2_instruction
{
	uint8_t  opcode;   // Opcode
	uint8_t  arg[3];   // Instr. arguments (0-3)   
	uint8_t  len;      // Instruction length
	uint32_t address;  // 32-bit address inside the code (jump to code_off)
	uint64_t constant; // 64-bit constant (immediate value)

    uint32_t code_off; // Offset inside the code section
};

/*
 * This table is indexed by group (first 3bits of the OP)
 * Results: index mask to use with the OP (to get the actual index)
 */
uint8_t op_grp_table[8] = {
	0, 0, 0x1C, 0x18, 0x18, 0x18, 0x10, 0x10
};

uint8_t op_index_shift[8] = {
	0, 0, 2, 3, 3, 3, 4, 4
};

struct instr_info instr_table[23] = {
	// mov
	INIT_INSTR_PROP(3, 13, 2, 0, 0), 
	// loadConst
	INIT_INSTR_PROP(3, 72, 1, 0, 1), 
	// add
	INIT_INSTR_PROP(6, 21, 3, 0, 0),
	// sub
	INIT_INSTR_PROP(6, 21, 3, 0, 0), 
	// div
	INIT_INSTR_PROP(6, 21, 3, 0, 0), 
	// mod	
	INIT_INSTR_PROP(6, 21, 3, 0, 0),
	// mul
	INIT_INSTR_PROP(6, 21, 3, 0, 0), 
	// compare
	INIT_INSTR_PROP(5, 20, 3, 0, 0), 
	// jump	
	INIT_INSTR_PROP(5, 37, 0, 1, 0),
	// jump equal
	INIT_INSTR_PROP(5, 47, 2, 1, 0),
	// read	
	INIT_INSTR_PROP(5, 25, 4, 0, 0), 
	// write
	INIT_INSTR_PROP(5, 20, 3, 0, 0),
	// consoleRead
	INIT_INSTR_PROP(5, 10, 1, 0, 0), 
	// consolwWrite
	INIT_INSTR_PROP(5, 10, 1, 0, 0), 
	// createThread
	INIT_INSTR_PROP(5, 42, 1, 1, 0),
	// joinThread	
	INIT_INSTR_PROP(5, 10, 1, 0, 0), 
	// hlt
	INIT_INSTR_PROP(5, 5, 0, 0, 0), 
	// sleep
	INIT_INSTR_PROP(5, 10, 1, 0, 0),
	// call
	INIT_INSTR_PROP(4, 34, 0, 1, 0), 
	// ret
	INIT_INSTR_PROP(4, 4, 0, 0, 0), 
	// lock	
	INIT_INSTR_PROP(4, 9, 1, 0, 0),
	// unlock
	INIT_INSTR_PROP(4, 9, 1, 0, 0) 
};

#endif
