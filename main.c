#include <stdio.h>
#include <malloc.h>
#include <dirent.h>
#include <string.h>

#include "esetvm2.h"

#ifdef ESETVM2_DISASSEMBLY
extern struct esetvm2_instr_decoded decode(struct esetvm2hdr *hdr);
#endif

#define PRECOMPILED_PATH "samples/precompiled/"
#define EVM ".evm"

// TODO menu to select the proper evm file

extern struct esetvm2 *vm;

int main(int argc, char *argv[]) {
	char file_path[50] = "samples/precompiled/";
	
	int ret = 0;
	if(argc < 2) {
		printf("Missing input file, exiting.\n");
		return -1;
	}

	strncat(file_path, argv[1], sizeof(argv[1]));
	strncat(file_path, EVM, 5);

	printf("Loading evm file %s...", file_path);

	FILE *fp = fopen(file_path, "rb");
	
	if(!fp) {
		printf("VM will load the proper evm file based on the name. Avoid entering the extension\n");
		return -2;
	}

	int size = file_size(fp);
	
	struct esetvm2hdr *eset_vm_hdr = vm_init(fp, size, argv[1]);

	if (!eset_vm_hdr) {
		printf("Invalid task header\n");
		ret = -3;
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
	if(vm->hbin)
		free(vm->hbin);
	// TODO free allocated memory

	return ret;
}


