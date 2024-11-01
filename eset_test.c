#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// leggere byte per byte partendo da dx: ogni byte è in little endian (va invertito, leggendo i singoli bit da dx a sx)
// https://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
#define MOV         0x00
#define LOAD_CONST  0x20
#define ADD         0x44
#define SUB         0x48
#define DIV         0x4C
#define MOD         0x50
#define MUL         0x54
#define COMPARE     0x60
#define JUMP        0x68
#define JUMP_EQUAL  0x70
#define READ        0x80
#define WRITE       0x88
#define CONS_READ   0x90
#define CONS_WRITE  0x98
#define C_THREAD    0xA0
#define J_THREAD    0xA8
#define HLT         0xB0
#define SLEEP       0xB8
#define CALL        0xC0
#define RET         0xD0
#define LOCK        0xE0
#define UNLOCK      0xF0

uint8_t opcode_map[22] = {
    MOV, LOAD_CONST, ADD, SUB, DIV, MOD, MUL, COMPARE, JUMP,
    JUMP_EQUAL, READ, WRITE, CONS_READ, CONS_WRITE, C_THREAD,
    J_THREAD, HLT, SLEEP, CALL, RET, LOCK, UNLOCK
};

struct instr_info
{
    uint8_t op_size;
    uint8_t len;
    uint8_t nr_args;
    uint8_t addr;
    uint8_t constant;
	char *mnemonic;
};

#define INIT_INSTR_PROP(_op_size, _len, _nr_args, _addr, _constant, _mnemonic) \
        {                                                           		\
            .op_size   = _op_size,                                  		\
            .len       = _len,                                      		\
            .nr_args   = _nr_args,                                 			\
            .addr      = _addr,                                     		\
            .constant  = _constant,                                  		\
			.mnemonic  = _mnemonic											\
        }  

struct instr_info instr_table[22] = {
    // mov
    INIT_INSTR_PROP(3, 13, 2, 0, 0, "mov"),
    // loadConst
    INIT_INSTR_PROP(3, 72, 1, 0, 1, "loadConst"),
    // add
    INIT_INSTR_PROP(6, 21, 3, 0, 0, "add"),
    // sub
    INIT_INSTR_PROP(6, 21, 3, 0, 0, "sub"),
    // div
    INIT_INSTR_PROP(6, 21, 3, 0, 0, "div"),
    // mod  
    INIT_INSTR_PROP(6, 21, 3, 0, 0, "mod"),
    // mul
    INIT_INSTR_PROP(6, 21, 3, 0, 0, "mul"),
    // compare
    INIT_INSTR_PROP(5, 20, 3, 0, 0, "compare"),
    // jump 
    INIT_INSTR_PROP(5, 37, 0, 1, 0, "jump"),
    // jump equal
    INIT_INSTR_PROP(5, 47, 2, 1, 0, "jumpEqual"),
    // read 
    INIT_INSTR_PROP(5, 25, 4, 0, 0, "read"),
    // write
    INIT_INSTR_PROP(5, 20, 3, 0, 0, "write"),
    // consoleRead
    INIT_INSTR_PROP(5, 10, 1, 0, 0, "consoleRead"),
    // consolwWrite
    INIT_INSTR_PROP(5, 10, 1, 0, 0, "consoleWrite"),
    // createThread
    INIT_INSTR_PROP(5, 42, 1, 1, 0, "createThread"),
    // joinThread   
    INIT_INSTR_PROP(5, 10, 1, 0, 0, "joinThread"),
    // hlt
    INIT_INSTR_PROP(5, 5, 0, 0, 0, "hlt"),
    // sleep
    INIT_INSTR_PROP(5, 10, 1, 0, 0, "sleep"),
    // call
    INIT_INSTR_PROP(4, 34, 0, 1, 0, "call"),
    // ret
    INIT_INSTR_PROP(4, 4, 0, 0, 0, "ret"),
    // lock 
    INIT_INSTR_PROP(4, 9, 1, 0, 0, "lock"),
    // unlock
    INIT_INSTR_PROP(4, 9, 1, 0, 0, "unlock")
};


uint8_t op_grp_table[8] = {
    0, 0, 0x1C, 0x18, 0x18, 0x18, 0x10, 0x10
};

uint8_t op_index_shift[8] = {
    0, 0, 2, 3, 3, 3, 4, 4
}; 


 
int main(){
	//char buffer[] = {0x3B, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x49, 0x04, 0x65, 0x43, 0xAD, 0x12, 0x00, 0x00 ,0x00};
	//char buffer[] = {0x49, 0x04, 0x65, 0x43, 0xAD, 0x12, 0x00, 0x00 ,0x00};
	char buffer[] = {0xA8, 0x60, 0x82, 0x30, 0x51, 0x53, 0x82, 0x77, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xB6};
	// Next instr -> previous_instr len / 8 = 10 / 8, so +1
	// 10 % 8 = 2, << 2
	// 25 / 8 = 3; 25 % 8 = 1 (2+1)
	// 20 / 8 = 2; 20 % 8 = 4 (4+2+1) -> greater then 3, so get missing bits from next byte
	// 72 / 8 = 9; 72 % 8 = 0 (4+2+1+0), like before
	// 4 / 8 = 0; 4 % 8   = 4 (4+4+2+1+0)
	// if reminder >= 8, skip byte: then reminder % 8 (shift)

// 00000101'10110110 

	uint8_t tmp = (buffer[16] << 3);
	printf("TMP: %x\n", tmp);
	
	uint8_t grp       = (tmp & 0xE0) >> 5;
	uint8_t grp_mask  = op_grp_table[grp];
	uint8_t grp_index = (tmp & grp_mask) >> op_index_shift[grp];

	printf("grp %d, mask: %x,  index grp: %d\n\n", grp,grp_mask, grp_index);	

	uint8_t op = (grp << 5) | (grp_index << op_index_shift[grp]);
	printf("op: %x\n", op);

	int i = 0;
	for(i=0; i<23; i++) {
		if(op == opcode_map[i]) 
			break;
	}

	struct instr_info info = instr_table[i];
	printf("len: %d\n", info.len);
	printf("op size: %d\n", info.op_size);
	printf("nr args: %d\n", info.nr_args);
	printf("constant: %d\n", info.constant);
	printf("address: %d\n", info.addr);
	printf("mnemonic: %s\n", info.mnemonic);

	return 0;

}
