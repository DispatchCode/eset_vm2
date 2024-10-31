#include <stdio.h>
#include <malloc.h>

#include "esetvm2hdr.h"
#include "esetvm2.h"

struct esetvm2 get_vm_instance(FILE * fp, int memory_size)
{
	struct esetvm2 eset_vm;

	eset_vm.ip = 0;
	eset_vm.memory = malloc(memory_size);

	return eset_vm;
}

struct esetvm2hdr * vm_load_task(struct esetvm2 *eset_vm, FILE *fp, int memory_size)
{
	fread(eset_vm->memory, sizeof(uint8_t), memory_size, fp);

	return load_task(eset_vm);
}

