#include <stdlib.h>
#include <string.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"
#include "common.h"


#ifdef DEBUG_PRINT_INSTR
static void print_decoded_instr(struct esetvm2_instruction instr) {
	struct instr_info info = instr_table[instr.op_table_index];

	printf("%3d:%15s\t", instr.code_off, info.mnemonic);

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

static struct esetvm2_instruction decode_instruction(struct esetvm2 *vm, int op_map_index) {
	struct esetvm2_instruction instr;
	struct instr_info info;
	int code_bit;

	// TODO info: could be changed using an enum for ARG, CONST, ADDR
	info = instr_table[op_map_index];
	instr.op_table_index = op_map_index;

	// TODO stored inside the VM ?
	code_bit = vm->ip + info.op_size;

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

#ifdef ESETVM2_DISASSEMBLY
struct esetvm2_instr_decoded decode(struct esetvm2hdr *hdr, struct esetvm2 *vm) {
	struct esetvm2_instr_decoded instr_decoded = INIT_INSTR_DECODED(10);
	int instr = 0;
	int code_size = hdr->code_size;
	int bit_code_size = CODE_OFFSET_BIT + (code_size * 8) - (code_size % 8);

	// TODO bugfix: wrong if padding is present
	while(vm->ip < bit_code_size)
	{
		struct esetvm2_instruction instr;
		uint8_t op = vm_next_op(vm);
		
		int op_map_index = get_op_map_index(op);
		// TODO check out of bounds

#ifdef DEBUG_INSTR_FORMAT
		print_internal_rapresentation(instr_table[op_map_index]);
#endif

		instr = decode_instruction(vm, op_map_index);

#ifdef DEBUG_PRINT_INSTR
		instr.code_off = vm->ip - CODE_OFFSET_BIT;
		print_decoded_instr(instr);
#endif

		vm_shift_ptr(vm, instr.len);

		PUSH_INSTR(instr_decoded, instr);	
	}

	return instr_decoded;
}
#else /* !ESETVM2_DISASSEMBLY */
struct esetvm2_instruction decode(struct esetvm2 *vm)
{
	struct esetvm2_instruction instr;

	uint8_t op = vm_next_op(vm);

	int op_map_index = get_op_map_index(op);
	instr = decode_instruction(vm, op_map_index);

#ifdef DEBUG_PRINT_INSTR
	print_decoded_instr(instr);
#endif

	return instr;
}

#endif
