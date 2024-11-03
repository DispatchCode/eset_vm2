#include <stdlib.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"

#define DEBUG_INSTR_FORMAT
#define DEBUG_PRINT_INSTR

#ifdef DEBUG_PRINT_INSTR
static void print_decoded_instr(struct esetvm2_instruction instr) {
	struct instr_info info = instr_table[instr.op_table_index];
	
	printf("%s \t", info.mnemonic);
	
	if(info.constant)
		printf("%ld", instr.constant);

	if(info.addr) {
		// TODO print address
	}	
	
	if(info.nr_args) printf(", ");

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

static int64_t read_const(struct esetvm2 *vm, int start, int num_bits) {
	int64_t constant = 0;
	for(int i=0; i < num_bits; i++) {
		constant = (constant << 1) | get_bit_at(vm, start + (num_bits - 1) - i);
	}
	return constant;
}

static struct esetvm2_instruction decode_instruction(struct esetvm2 *vm, struct instr_info info) {
	struct esetvm2_instruction instr;
	int code_bit;

	instr.op_table_index = info.op_table_index;
	code_bit = CODE_OFFSET_BIT + info.code_offset + info.op_size;
	
	if (info.constant) {
		//printf("code_offset: %d, op size: %d\n", info.code_offset, info.op_size);
		instr.constant = read_const(vm, code_bit, 64);
		code_bit += 64;
	}

	if(info.addr) {
		// TODO address parsing
	}

	if(info.nr_args) {
		for(int i=0; i < info.nr_args; i++) {
			int arg;
			switch(get_bit_at(vm, code_bit)) {
				case 0:
					arg = read_const(vm, code_bit+1, 4);	
				case 1:
				// TODO argx, ss, xxxx
			}

			instr.arg[i] = arg;
			//printf("Register nr: %d\n", arg);
		}
	}

	return instr;
}

struct esetvm2_instruction *decode(struct esetvm2hdr *hdr, struct esetvm2 *vm) {
	// TODO use the correct CODE size
	struct esetvm2_instruction *instructions = calloc(1000, sizeof(struct esetvm2_instruction));
	int buff_index = CODE_OFFSET;
	int buff_shift = 0;
	int instr = 0;
	int tot_instr = hdr->code_size + CODE_OFFSET;
   	int code_off = 0;
 
	while(buff_index < tot_instr)
	{
		uint8_t tmp;

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

        int i = -1;
        while(++i < sizeof(opcode_map) && op != opcode_map[i]);
		// TODO check out of bounds
		//printf("Index OPcode Map: %d\n", i);
		// Internal rappresentation of an instruction,
		// used to describe the format
        struct instr_info info = instr_table[i];
#ifdef DEBUG_INSTR_FORMAT
		print_internal_rapresentation(info);
#endif

		info.memory_index = buff_index;
		info.op_table_index = i;
		info.code_offset = code_off;		
		code_off += info.len;

		instructions[instr++] = decode_instruction(vm, info);
#ifdef DEBUG_PRINT_INSTR
		print_decoded_instr(instructions[instr-1]);
#endif		

        buff_index += (info.len >> 3);
        buff_shift += (info.len % 8);
		
		break;
	}

    return instructions;
}
