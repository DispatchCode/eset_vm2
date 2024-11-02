#include "esetvm2hdr.h"
#include "esetvm2.h"
#include "esetvm2decode.h"

#ifdef DEBUG_INSTR_FORMAT
static void print_internal_rapresentation(struct instr_info info) {
	printf("%s ", info.mnemonic);
	
}
#endif


static esetvm2_instruction decode_instruction(struct instr_info info) {
	
}

struct esetvm2_instruction decode(struct esetvm2hdr *hdr) {
	struct esetvm2_instruction *instructions = calloc(hdr->code_size, sizeof(struct esetvm2_instruction));
	int buff_index = 0;
	int buff_size  = sizeof(buffer);
	int buff_shift = 0;
	int instr = 0;
	int tot_instr = hdr->code_size;
    
	while(instr < tot_instr)
	{
		uint8_t tmp;

		// the opcode is inside the first byte
		if (buff_shift < 4) {
			tmp = buffer[buff_index] << buff_shift;
        }
		// OP is partially in the first byte, so we need bits from the next byte
        else if (buff_shift < 8 && buff_shift > 3) {
            tmp = (buffer[buff_index] << buff_shift) | (buffer[buff_index+1] >> (8 - buff_shift));
        }
		// remainder >=8, so we can discard the byte and take the next
        else {
            buff_shift %= 8;
            tmp = buffer[++buff_index] << buff_shift;
        }

        // get the "group" from the opcode
        uint8_t grp = (tmp & 0xE0) >> 5;
        // mask of the group (used to retrieve the index)
        uint8_t grp_mask = op_grp_table[grp];
        // the "index" part of the opcode
        uint8_t grp_index = (tmp & grp_mask) >> op_index_shift[grp];

		// byte aligned opcode, like in the opcode_map
        uint8_t op = (grp << 5) | (grp_index << op_index_shift[grp]);

        int i = -1;
        while(i++ < sizeof(opcode_map) && op != opcode_map[i]);
		// TODO check out of bounds

		// Internal rappresentation of an instruction,
		// used to describe the format
        struct instr_info info = instr_table[i];

		uint8_t old_buff_index = buff_index + 1;
		
        buff_index += (info.len >> 3);
        buff_shift += (info.len % 8);
		
		// Copy raw data from the buffer into info.rawdata
		int raw_index = 1;
		info.rawdata[0] = buffer[old_buff_index] ^ op;
		while(old_buff_index < buff_index) {
			info.raw_data[raw_index++] = buffer[old_buff_index++];
		}
		// copy bits from the latest byte part of the instr.
		// TODO copy bits using "info" (eg. nr of bits of the opcode, nr of arguments etc)
		//	OR -> instruction length - opcode length = bits to copy in the raw_data array 

#ifdef DEBUG_INSTR_FORMAT
		print_internal_rapresentatino(info);
#endif
	
		instructions[instr++] = decode_instruction(info);

	}

    return instructions;
}
