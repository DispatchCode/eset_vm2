#include <stdio.h>
#include <malloc.h>
#include <dirent.h>

#include "esetvm2.h"

#ifdef ESETVM2_DISASSEMBLY
extern struct esetvm2_instr_decoded decode(struct esetvm2hdr *hdr);
#endif

extern uint8_t *memory;
extern struct esetvm2 *vm;


int main() {
	int ret = 0;
	FILE *fp = fopen("thread.evm", "rb");
	
	if(!fp) {
		printf(".evm file not found, abort.\n");
		return -1;
	}

	int size = file_size(fp);
	init_vm_instance(fp, size);
	
	struct esetvm2hdr *eset_vm_hdr = vm_load_task(fp, size);

	if (!eset_vm_hdr) {
		printf("Invalid task header\n");
		ret = -1;
		goto clean;
	}

	print_task_hdr(eset_vm_hdr);
#ifdef ESETVM2_DISASSEMBLY
	struct esetvm2_instr_decoded decoded_instr = decode(eset_vm_hdr);
	printf("Instr decoded: %d\n", decoded_instr.tos);

#else

	vm_start();

#endif

clean:
	if (memory)
		free(memory);

	return ret;
}


