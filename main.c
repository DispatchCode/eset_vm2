#include <stdio.h>
#include <malloc.h>
#include <dirent.h>

#include "esetvm2.h"

#ifdef ESETVM2_DISASSEMBLY
extern struct esetvm2_instr_decoded decode(struct esetvm2hdr *hdr, struct esetvm2 *);
#endif

extern uint8_t *memory;

int main() {
	int ret = 0;
	FILE *fp = fopen("samples/precompiled/math2.evm", "rb");
	
	if(!fp) {
		printf(".evm file not found, abort.\n");
		return -1;
	}

	int size = file_size(fp);
	struct esetvm2 eset_vm = get_vm_instance(fp, size);
	
	struct esetvm2hdr *eset_vm_hdr = vm_load_task(&eset_vm, fp, size);

	if (!eset_vm_hdr) {
		printf("Invalid task header\n");
		ret = -1;
		goto clean;
	}

	print_task_hdr(eset_vm_hdr);
#ifdef ESETVM2_DISASSEMBLY
	struct esetvm2_instr_decoded decoded_instr = decode(eset_vm_hdr, &eset_vm);
	printf("Instr decoded: %d\n", decoded_instr.tos);

#else

	vm_start(&eset_vm);

#endif

clean:
	if (memory)
		free(memory);

	return ret;
}


