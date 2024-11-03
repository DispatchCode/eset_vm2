#include <stdio.h>
#include <malloc.h>
#include <dirent.h>

#include "esetvm2.h"

extern struct esetvm2_instruction *decode(struct esetvm2hdr *hdr, struct esetvm2 *);

int main() {
	int ret = 0;
	FILE *fp = fopen("test.evm", "rb");
	int size = file_size(fp);

	struct esetvm2 eset_vm = get_vm_instance(fp, size);
	struct esetvm2hdr *eset_vm_hdr = vm_load_task(&eset_vm, fp, size);

	if (!eset_vm_hdr) {
		printf("Invalid task header\n");
		ret = -1;
		goto clean;
	}

	print_task_hdr(eset_vm_hdr);
	
	printf("\n\t[ Decoding instructions... ]\n\n");	
	struct esetvm2_instruction *instructions = decode(eset_vm_hdr, &eset_vm);
	
clean:
	if (eset_vm.memory)
		free(eset_vm.memory);

	return ret;
}


