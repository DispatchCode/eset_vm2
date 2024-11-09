#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "esetvm2.h"

extern uint8_t *memory;
extern int memory_size;

void print_task_hdr(struct esetvm2hdr * vm_hdr)
{
	printf("=========== Task Header ===========\n");
	printf("Magic: ");
	for(int i=0; i<sizeof(vm_hdr->magic); i++)
		printf("%c", vm_hdr->magic[i]);
	
	printf("\nSize of code (bytes): %d\n", vm_hdr->code_size);
	printf("Size of data (bytes): %d\n", vm_hdr->data_size);
	printf("Size of initial data (bytes): %d\n", vm_hdr->initial_data_size);
	printf("================================\n");
}

int file_size(FILE *fp) 
{
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return size;
}

uint32_t code_size(struct esetvm2 vm)
{

}

uint32_t data_size(struct esetvm2 vm)
{

}

struct esetvm2hdr *load_task(struct esetvm2 *vm)
{
	struct esetvm2hdr *eset_hdr;
	int valid;

	eset_hdr = malloc(sizeof(struct esetvm2hdr));
	memcpy(&eset_hdr->magic, memory, 8);
	memcpy(&eset_hdr->code_size, memory+8, sizeof(uint32_t));
	memcpy(&eset_hdr->data_size, memory+12, sizeof(uint32_t));
	memcpy(&eset_hdr->initial_data_size, memory+16, sizeof(uint32_t));

	valid = strcmp(eset_hdr->magic, MAGIC) && eset_hdr->data_size >= eset_hdr->initial_data_size;
	
	return valid ? eset_hdr : NULL;
}

