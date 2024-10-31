#ifndef ESETVM2_HDR
#define ESETVM2_HDR

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "esetvm2.h"

#define MAGIC "ESET-VM2"

#define START_OF_CODE 0x14


struct esetvm2hdr
{
	int8_t magic[8];
	uint32_t code_size;         // in instructions
	uint32_t data_size;         // bytes
	uint32_t initial_data_size;	// bytes
};

bool is_header_valid();
uint32_t code_size();
uint32_t data_size();

int file_size(FILE *fp);
void print_task_hdr(struct esetvm2hdr *);
struct esetvm2hdr* load_task(struct esetvm2 *vm);
#endif
