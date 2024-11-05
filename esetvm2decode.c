#include <stdlib.h>
#include <string.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"

// #define DEBUG_INSTR_FORMAT
#define DEBUG_PRINT_INSTR

#ifdef DEBUG_PRINT_INSTR
static void print_decoded_instr(struct esetvm2_instruction instr) {
	struct instr_info info = instr_table[instr.op_table_index];

	printf("%3d:%10s\t", instr.code_off, info.mnemonic);

	if(info.constant)
		printf("%ld", instr.constant);

	if(info.addr) {
		printf("%d", instr.address);
	}	

	if((info.constant + info.addr) && info.nr_args) printf(", ");

	for(int i=0; i<info.nr_args; i++) {
		printf("r%d%c", instr.arg[i], (i != info.nr_args-1) ? ',' : ' ');
	}

	printf("\n");
}
#endif

#ifdef DEBUG_INSTR_FORMAT
static void print_internal_rapresentation(struct instr_info info) {
	printf("%s(%x) \t", info.mnemonic, opcode_map[info.op_table_index]);

	if (info.constant)
		printf("constant");

	if (info.addr)
		printf("address");

	if (info.nr_args) printf(", ");

	for(int i=0; i<info.nr_args; i++) 
	{
		printf("arg%d%c", i+1, (i != info.nr_args-1) ? ',' : ' ');
	}

	printf("\n");
}
#endif


static inline int get_bit_at(struct esetvm2 *vm, int pos)
{
	int byte_index = pos >> 3;
	int bit_index  = pos % 8;
	return (vm->memory[byte_index] >> (7 - bit_index) & 1);
}

static inline int64_t read_const(struct esetvm2 *vm, int start, int num_bits) {
	int64_t constant = 0;
	for(int i=0; i < num_bits; i++) {
		constant = (constant << 1) | get_bit_at(vm, start + (num_bits - 1) - i);
	}
	return constant;
}

static struct esetvm2_instruction decode_instruction(struct esetvm2 *vm, int op_map_index, int code_offset) {
	struct esetvm2_instruction instr;
	struct instr_info info;
	int code_bit;

	// TODO info: could be changed using an enum for ARG, CONST, ADDR
	info = instr_table[op_map_index];
	instr.op_table_index = op_map_index;

	// TODO stored inside the VM ?
	code_bit = CODE_OFFSET_BIT + code_offset + info.op_size;

	instr.code_off = code_offset;

	if (info.constant) {
		//printf("code_offset: %d, op size: %d\n", info.code_offset, info.op_size);
		instr.constant = read_const(vm, code_bit, 64);
		code_bit += 64;
	}

	if(info.addr) {
		instr.address = read_const(vm, code_bit, 32);
		code_bit += 32;
	}

	for(int i=0; i < info.nr_args; i++) {
		switch(get_bit_at(vm, code_bit)) {
			case 0:
				instr.arg[i] = read_const(vm, code_bit+1, 4);
				code_bit += 5;
				break;	
			case 1:
				// TODO argx, ss, xxxx
				break;
		}
	}

	instr.len = info.len;

	return instr;
}

static inline int get_op_map_index(uint8_t opcode) {
	int i = -1;
	while(++i < sizeof(opcode_map) && opcode != opcode_map[i]);
	return i;
}

struct esetvm2_instr_decoded decode(struct esetvm2hdr *hdr, struct esetvm2 *vm) {
	struct esetvm2_instr_decoded instr_decoded = INIT_INSTR_DECODED(10);
	int buff_index = CODE_OFFSET;
	int buff_shift = 0;
	int instr = 0;
	int code_size = hdr->code_size;
	int code_off = 0;

	int stop_after = 0;
	while(code_off >> 3  < code_size-1)
	{
		uint8_t tmp;
		struct esetvm2_instruction instr;

		// the opcode is inside the first byte
		if (buff_shift < 4) {
			tmp = vm->memory[buff_index] << buff_shift;
		}
		// OP is partially in the first byte, so we need bits from the next byte
		else if (buff_shift < 8 && buff_shift > 3) {
			tmp = (vm->memory[buff_index] << buff_shift) | (vm->memory[buff_index+1] >> (8 - buff_shift));
		}
		// remainder >=8, so we can discard the byte and take the next
		else {
			buff_shift %= 8;
			tmp = vm->memory[++buff_index] << buff_shift;
		}

		// get the "group" from the opcode
		uint8_t grp = (tmp & 0xE0) >> 5;
		// mask of the group (used to retrieve the index)
		uint8_t grp_mask = op_grp_table[grp];
		// the "index" part of the opcode
		uint8_t grp_index = (tmp & grp_mask) >> op_index_shift[grp];

		// byte aligned opcode, like in the opcode_map
		uint8_t op = (grp << 5) | (grp_index << op_index_shift[grp]);
		//printf("opcode: %x, opcode map sizeof: %d\n", op, sizeof(opcode_map));

		int op_map_index = get_op_map_index(op);

		// TODO check out of bounds
		//printf("Index OPcode Map: %d\n", i);

#ifdef DEBUG_INSTR_FORMAT
		print_internal_rapresentation(instr_table[op_map_index]);
#endif

		instr = decode_instruction(vm, op_map_index, code_off);

#ifdef DEBUG_PRINT_INSTR
		print_decoded_instr(instr);
#endif

		buff_index += (instr.len >> 3);
		buff_shift += (instr.len % 8);

		code_off += instr.len;

		PUSH_INSTR(instr_decoded, instr);	

	}

	return instr_decoded;
}
