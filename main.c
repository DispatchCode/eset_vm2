#include <stdio.h>
#include <malloc.h>
#include <dirent.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"


int main() {
	int ret = 0;
	FILE *fp = fopen("math.evm", "rb");
	int size = file_size(fp);

	struct esetvm2 eset_vm = get_vm_instance(fp, size);
	struct esetvm2hdr *eset_vm_hdr = vm_load_task(&eset_vm, fp, size);

	if (!eset_vm_hdr) {
		printf("Invalid task header\n");
		ret = -1;
		goto clean;
	}

	print_task_hdr(eset_vm_hdr);
	
	struct esetvm2_instruction *instructions = calloc(eset_vm_hdr->code_size, sizeof(struct esetvm2_instruction));
	printf("Decoding instructions...\n");	
	
clean:
	if (eset_vm.memory)
		free(eset_vm.memory);

	return ret;
}


